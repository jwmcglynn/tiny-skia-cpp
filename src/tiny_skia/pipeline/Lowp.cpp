// Disable FMA contraction to match Rust's lowp pipeline, which uses software
// SIMD wrappers (f32x16/f32x8) that prevent LLVM from fusing multiply-add.
#pragma clang fp contract(off)

#include "tiny_skia/pipeline/Lowp.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/wide/F32x16T.h"
#include "tiny_skia/wide/F32x8T.h"
#include "tiny_skia/wide/U16x16T.h"
#include "tiny_skia/wide/backend/Aarch64NeonU16x16T.h"
#include "tiny_skia/wide/backend/X86Avx2FmaU16x16T.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <span>

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

namespace tiny_skia::pipeline::lowp {

using tiny_skia::wide::F32x16T;
using tiny_skia::wide::F32x8T;
using tiny_skia::wide::U16x16T;

// Maps to Rust's u16x16 — a 16-element channel of lowp pixel values in [0, 255].
using LowpChannel = U16x16T;

struct Pipeline {
  LowpChannel r{};
  LowpChannel g{};
  LowpChannel b{};
  LowpChannel a{};
  LowpChannel uniform_r{};
  LowpChannel uniform_g{};
  LowpChannel uniform_b{};
  LowpChannel uniform_a{};
  LowpChannel dr{};
  LowpChannel dg{};
  LowpChannel db{};
  LowpChannel da{};

  const std::array<StageFn, tiny_skia::pipeline::kMaxStages>* functions = nullptr;
  std::size_t index = 0;
  std::size_t dx = 0;
  std::size_t dy = 0;
  std::size_t tail = 0;
  const ScreenIntRect* rect = nullptr;
  const AAMaskCtx* aa_mask_ctx = nullptr;
  const MaskCtx* mask_ctx = nullptr;
  Context* ctx = nullptr;
  SubPixmapMut* pixmap_dst = nullptr;

  Pipeline(const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& fun,
           const std::array<StageFn, tiny_skia::pipeline::kMaxStages>&
               tail_fun,
           const ScreenIntRect& rect_arg,
           const AAMaskCtx& aa_mask_ctx_arg,
           const MaskCtx& mask_ctx_arg,
           Context& ctx_arg,
           SubPixmapMut* pixmap_dst_arg)
      : uniform_r(U16x16T::splat(ctx_arg.uniform_color.rgba[0])),
        uniform_g(U16x16T::splat(ctx_arg.uniform_color.rgba[1])),
        uniform_b(U16x16T::splat(ctx_arg.uniform_color.rgba[2])),
        uniform_a(U16x16T::splat(ctx_arg.uniform_color.rgba[3])),
        functions(&fun),
        rect(&rect_arg),
        aa_mask_ctx(&aa_mask_ctx_arg),
        mask_ctx(&mask_ctx_arg),
        ctx(&ctx_arg),
        pixmap_dst(pixmap_dst_arg) {
    (void)tail_fun;
  }

  void nextStage() {
    const auto next = (*functions)[index];
    ++index;
    next(*this);
  }
};

bool fnPtrEq(StageFn a, StageFn b) {
  return a == b;
}

const void* fnPtr(StageFn fn) {
  return reinterpret_cast<const void*>(fn);
}

namespace {

[[nodiscard]] constexpr bool useAarch64NeonNative() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return true;
#else
  return false;
#endif
}

[[nodiscard]] constexpr bool useX86Avx2FmaNative() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return true;
#else
  return false;
#endif
}

// Matches Rust split(): reinterpret f32x16 (64 bytes) as two u16x16 (32 bytes each)
inline void split(const F32x16T& v, U16x16T& lo, U16x16T& hi) {
  auto lo_lanes = v.lo().lanes();
  auto hi_lanes = v.hi().lanes();
  std::memcpy(&lo.lanes(), &lo_lanes, sizeof(lo.lanes()));
  std::memcpy(&hi.lanes(), &hi_lanes, sizeof(hi.lanes()));
}

// Matches Rust join(): reinterpret two u16x16 (32 bytes each) as f32x16 (64 bytes)
inline F32x16T join(const U16x16T& lo, const U16x16T& hi) {
  std::array<float, 8> lo_f{}, hi_f{};
  std::memcpy(&lo_f, &lo.lanes(), sizeof(lo_f));
  std::memcpy(&hi_f, &hi.lanes(), sizeof(hi_f));
  return F32x16T(F32x8T(lo_f), F32x8T(hi_f));
}

// Matches Rust lowp: (v + 255) >> 8
inline U16x16T div255(U16x16T v) {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return wide::backend::aarch64_neon::u16x16Div255(v);
#elif defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return wide::backend::x86_avx2_fma::u16x16Div255(v);
#else
  return (v + U16x16T::splat(255)) >> U16x16T::splat(8);
#endif
}

inline U16x16T mulDiv255(const U16x16T& lhs, const U16x16T& rhs) {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return wide::backend::aarch64_neon::u16x16MulDiv255(lhs, rhs);
#elif defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return wide::backend::x86_avx2_fma::u16x16MulDiv255(lhs, rhs);
#else
  return div255(lhs * rhs);
#endif
}

inline U16x16T mulAddDiv255(const U16x16T& lhs0, const U16x16T& rhs0,
                            const U16x16T& lhs1, const U16x16T& rhs1) {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return wide::backend::aarch64_neon::u16x16MulAddDiv255(lhs0, rhs0, lhs1, rhs1);
#elif defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return wide::backend::x86_avx2_fma::u16x16MulAddDiv255(lhs0, rhs0, lhs1, rhs1);
#else
  return div255(lhs0 * rhs0 + lhs1 * rhs1);
#endif
}

inline U16x16T sourceOverChannel(const U16x16T& source, const U16x16T& dest,
                                 const U16x16T& sourceAlpha) {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return wide::backend::aarch64_neon::u16x16SourceOver(source, dest, sourceAlpha);
#elif defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return wide::backend::x86_avx2_fma::u16x16SourceOver(source, dest, sourceAlpha);
#else
  return source + div255(dest * (U16x16T::splat(255) - sourceAlpha));
#endif
}

// Matches Rust from_float(f): (f * 255.0 + 0.5) as u16, splatted to all lanes
inline U16x16T fromFloat(float f) {
  return U16x16T::splat(static_cast<std::uint16_t>(f * 255.0f + 0.5f));
}

// Matches Rust round_f32_to_u16(): normalize, scale to [0,255], truncate
inline void roundF32ToU16(F32x16T rf, F32x16T gf, F32x16T bf, F32x16T af,
                          U16x16T& r, U16x16T& g, U16x16T& b, U16x16T& a) {
  rf = rf.normalize() * F32x16T::splat(255.0f) + F32x16T::splat(0.5f);
  gf = gf.normalize() * F32x16T::splat(255.0f) + F32x16T::splat(0.5f);
  bf = bf.normalize() * F32x16T::splat(255.0f) + F32x16T::splat(0.5f);
  af = af * F32x16T::splat(255.0f) + F32x16T::splat(0.5f);
  rf.saveToU16x16(r);
  gf.saveToU16x16(g);
  bf.saveToU16x16(b);
  af.saveToU16x16(a);
}

void move_source_to_destination(Pipeline& pipeline) {
  pipeline.dr = pipeline.r;
  pipeline.dg = pipeline.g;
  pipeline.db = pipeline.b;
  pipeline.da = pipeline.a;
  pipeline.nextStage();
}

void move_destination_to_source(Pipeline& pipeline) {
  pipeline.r = pipeline.dr;
  pipeline.g = pipeline.dg;
  pipeline.b = pipeline.db;
  pipeline.a = pipeline.da;
  pipeline.nextStage();
}

void premultiply(Pipeline& pipeline) {
  pipeline.r = mulDiv255(pipeline.r, pipeline.a);
  pipeline.g = mulDiv255(pipeline.g, pipeline.a);
  pipeline.b = mulDiv255(pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void uniform_color(Pipeline& pipeline) {
  pipeline.r = pipeline.uniform_r;
  pipeline.g = pipeline.uniform_g;
  pipeline.b = pipeline.uniform_b;
  pipeline.a = pipeline.uniform_a;
  pipeline.nextStage();
}

void seed_shader(Pipeline& pipeline) {
  const auto iota = F32x16T(
      F32x8T({0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f}),
      F32x8T({8.5f, 9.5f, 10.5f, 11.5f, 12.5f, 13.5f, 14.5f, 15.5f}));
  const auto x = F32x16T::splat(static_cast<float>(pipeline.dx)) + iota;
  const auto y = F32x16T::splat(static_cast<float>(pipeline.dy) + 0.5f);
  split(x, pipeline.r, pipeline.g);
  split(y, pipeline.b, pipeline.a);
  pipeline.dr = U16x16T::splat(0);
  pipeline.dg = U16x16T::splat(0);
  pipeline.db = U16x16T::splat(0);
  pipeline.da = U16x16T::splat(0);
  pipeline.nextStage();
}

void scale_u8(Pipeline& pipeline) {
  const auto data = pipeline.aa_mask_ctx->copyAtXY(pipeline.dx, pipeline.dy, pipeline.tail);
  U16x16T c{};
  auto& cl = c.lanes();
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    cl[i] = i < 2 ? static_cast<std::uint16_t>(data[i]) : 0;
  }
  pipeline.r = mulDiv255(pipeline.r, c);
  pipeline.g = mulDiv255(pipeline.g, c);
  pipeline.b = mulDiv255(pipeline.b, c);
  pipeline.a = mulDiv255(pipeline.a, c);
  pipeline.nextStage();
}

void lerp_u8(Pipeline& pipeline) {
  const auto data = pipeline.aa_mask_ctx->copyAtXY(pipeline.dx, pipeline.dy, pipeline.tail);
  U16x16T c{};
  auto& cl = c.lanes();
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    cl[i] = i < 2 ? static_cast<std::uint16_t>(data[i]) : 0;
  }
  const auto inv_c = U16x16T::splat(255) - c;
  pipeline.r = mulAddDiv255(pipeline.dr, inv_c, pipeline.r, c);
  pipeline.g = mulAddDiv255(pipeline.dg, inv_c, pipeline.g, c);
  pipeline.b = mulAddDiv255(pipeline.db, inv_c, pipeline.b, c);
  pipeline.a = mulAddDiv255(pipeline.da, inv_c, pipeline.a, c);
  pipeline.nextStage();
}

void scale_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  pipeline.r = mulDiv255(pipeline.r, c);
  pipeline.g = mulDiv255(pipeline.g, c);
  pipeline.b = mulDiv255(pipeline.b, c);
  pipeline.a = mulDiv255(pipeline.a, c);
  pipeline.nextStage();
}

void lerp_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  const auto inv_c = U16x16T::splat(255) - c;
  pipeline.r = mulAddDiv255(pipeline.dr, inv_c, pipeline.r, c);
  pipeline.g = mulAddDiv255(pipeline.dg, inv_c, pipeline.g, c);
  pipeline.b = mulAddDiv255(pipeline.db, inv_c, pipeline.b, c);
  pipeline.a = mulAddDiv255(pipeline.da, inv_c, pipeline.a, c);
  pipeline.nextStage();
}

// --- Blend-mode helpers (mirrors Rust's blend_fn! / blend_fn2! macros) ---
//
// blend_fn:  applies F(s, d, sa, da) uniformly to all four channels (r, g, b, a).
// blend_fn2: applies F(s, d, sa, da) to r, g, b only; alpha uses source_over:
//            a' = sa + div255(da * (255 - sa)).

template <typename F>
void blend_fn(Pipeline& pipeline, F&& f) {
  const auto sa = pipeline.a, da = pipeline.da;
  pipeline.r = f(pipeline.r, pipeline.dr, sa, da);
  pipeline.g = f(pipeline.g, pipeline.dg, sa, da);
  pipeline.b = f(pipeline.b, pipeline.db, sa, da);
  pipeline.a = f(sa, da, sa, da);
  pipeline.nextStage();
}

template <typename F>
void blend_fn2(Pipeline& pipeline, F&& f) {
  const auto sa = pipeline.a, da = pipeline.da;
  pipeline.r = f(pipeline.r, pipeline.dr, sa, da);
  pipeline.g = f(pipeline.g, pipeline.dg, sa, da);
  pipeline.b = f(pipeline.b, pipeline.db, sa, da);
  pipeline.a = sourceOverChannel(sa, da, sa);
  pipeline.nextStage();
}

void clear(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T /*d*/, U16x16T /*sa*/, U16x16T /*da*/) {
    return U16x16T::splat(0);
  });
}

void destination_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return mulAddDiv255(d, sa, s, U16x16T::splat(255) - da);
  });
}

void destination_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return mulDiv255(d, sa);
  });
}

void destination_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return mulDiv255(d, U16x16T::splat(255) - sa);
  });
}

void source_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return mulAddDiv255(s, da, d, U16x16T::splat(255) - sa);
  });
}

void source_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T /*d*/, U16x16T /*sa*/, U16x16T da) {
    return mulDiv255(s, da);
  });
}

void source_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T /*d*/, U16x16T /*sa*/, U16x16T da) {
    return mulDiv255(s, U16x16T::splat(255) - da);
  });
}

void source_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return sourceOverChannel(s, d, sa);
  });
}

void destination_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T da) {
    return sourceOverChannel(d, s, da);
  });
}

void modulate(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T /*da*/) {
    return mulDiv255(s, d);
  });
}

void multiply(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return div255(s * (U16x16T::splat(255) - da) + d * (U16x16T::splat(255) - sa) + s * d);
  });
}

void plus(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T /*da*/) {
    return (s + d).min(U16x16T::splat(255));
  });
}

void screen(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T /*da*/) {
    return s + d - mulDiv255(s, d);
  });
}

void x_or(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return mulAddDiv255(s, U16x16T::splat(255) - da, d, U16x16T::splat(255) - sa);
  });
}

void null_fn(Pipeline& pipeline) {
  (void)pipeline;
}


// Returns a span of PremultipliedColorU8 pixels starting at (dx, dy) in the pixmap.
// Matches Rust's pixmap.slice_at_xy(dx, dy) / pixmap.slice16_at_xy(dx, dy).
inline std::span<PremultipliedColorU8> pixelsAtXY(SubPixmapMut& pixmap,
                                                  std::size_t dx,
                                                  std::size_t dy) {
  const auto pixel_offset = dy * pixmap.real_width + dx;
  auto* pixels = reinterpret_cast<PremultipliedColorU8*>(pixmap.data);
  const auto total_pixels = pixmap.real_width * pixmap.size.height();
  return std::span<PremultipliedColorU8>(pixels + pixel_offset,
                                         total_pixels - pixel_offset);
}

// Matches Rust load_8888(&[PremultipliedColorU8; STAGE_WIDTH], ...).
// Loads u8 pixel channels into U16x16T (zero-extend u8 -> u16).
void load_8888_lowp(std::span<const PremultipliedColorU8> pixels,
                    U16x16T& or_, U16x16T& og,
                    U16x16T& ob, U16x16T& oa) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  if constexpr (useAarch64NeonNative()) {
    static_assert(sizeof(PremultipliedColorU8) == 4);

    const auto* packed = reinterpret_cast<const std::uint8_t*>(pixels.data());
    const uint8x16x4_t rgba = vld4q_u8(packed);
    const uint16x8x2_t r16 = {vmovl_u8(vget_low_u8(rgba.val[0])),
                              vmovl_u8(vget_high_u8(rgba.val[0]))};
    const uint16x8x2_t g16 = {vmovl_u8(vget_low_u8(rgba.val[1])),
                              vmovl_u8(vget_high_u8(rgba.val[1]))};
    const uint16x8x2_t b16 = {vmovl_u8(vget_low_u8(rgba.val[2])),
                              vmovl_u8(vget_high_u8(rgba.val[2]))};
    const uint16x8x2_t a16 = {vmovl_u8(vget_low_u8(rgba.val[3])),
                              vmovl_u8(vget_high_u8(rgba.val[3]))};
    vst1q_u16_x2(or_.lanes().data(), r16);
    vst1q_u16_x2(og.lanes().data(), g16);
    vst1q_u16_x2(ob.lanes().data(), b16);
    vst1q_u16_x2(oa.lanes().data(), a16);
    return;
  }
#endif

#if defined(__x86_64__) || defined(__i386__)
  if constexpr (useX86Avx2FmaNative()) {
    static_assert(sizeof(PremultipliedColorU8) == 4);

    const auto* packed = reinterpret_cast<const std::uint8_t*>(pixels.data());
    const __m128i p0 =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(packed + 0));
    const __m128i p1 =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(packed + 16));
    const __m128i p2 =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(packed + 32));
    const __m128i p3 =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(packed + 48));
    const __m128i zero = _mm_setzero_si128();

    const __m128i r_mask =
        _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i g_mask =
        _mm_setr_epi8(1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i b_mask =
        _mm_setr_epi8(2, 6, 10, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i a_mask =
        _mm_setr_epi8(3, 7, 11, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    const auto gather_channel = [&](__m128i mask) {
      const __m128i c0 = _mm_shuffle_epi8(p0, mask);
      const __m128i c1 = _mm_shuffle_epi8(p1, mask);
      const __m128i c2 = _mm_shuffle_epi8(p2, mask);
      const __m128i c3 = _mm_shuffle_epi8(p3, mask);
      const __m128i c01 = _mm_unpacklo_epi32(c0, c1);
      const __m128i c23 = _mm_unpacklo_epi32(c2, c3);
      return _mm_unpacklo_epi64(c01, c23);
    };

    const __m128i r8 = gather_channel(r_mask);
    const __m128i g8 = gather_channel(g_mask);
    const __m128i b8 = gather_channel(b_mask);
    const __m128i a8 = gather_channel(a_mask);

    _mm_storeu_si128(reinterpret_cast<__m128i*>(or_.lanes().data()),
                     _mm_unpacklo_epi8(r8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(or_.lanes().data() + 8),
                     _mm_unpackhi_epi8(r8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(og.lanes().data()),
                     _mm_unpacklo_epi8(g8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(og.lanes().data() + 8),
                     _mm_unpackhi_epi8(g8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ob.lanes().data()),
                     _mm_unpacklo_epi8(b8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ob.lanes().data() + 8),
                     _mm_unpackhi_epi8(b8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(oa.lanes().data()),
                     _mm_unpacklo_epi8(a8, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(oa.lanes().data() + 8),
                     _mm_unpackhi_epi8(a8, zero));
    return;
  }
#endif

  auto& rl = or_.lanes();
  auto& gl = og.lanes();
  auto& bl = ob.lanes();
  auto& al = oa.lanes();

  rl[0] = pixels[0].red();    rl[1] = pixels[1].red();
  rl[2] = pixels[2].red();    rl[3] = pixels[3].red();
  rl[4] = pixels[4].red();    rl[5] = pixels[5].red();
  rl[6] = pixels[6].red();    rl[7] = pixels[7].red();
  rl[8] = pixels[8].red();    rl[9] = pixels[9].red();
  rl[10] = pixels[10].red();  rl[11] = pixels[11].red();
  rl[12] = pixels[12].red();  rl[13] = pixels[13].red();
  rl[14] = pixels[14].red();  rl[15] = pixels[15].red();

  gl[0] = pixels[0].green();    gl[1] = pixels[1].green();
  gl[2] = pixels[2].green();    gl[3] = pixels[3].green();
  gl[4] = pixels[4].green();    gl[5] = pixels[5].green();
  gl[6] = pixels[6].green();    gl[7] = pixels[7].green();
  gl[8] = pixels[8].green();    gl[9] = pixels[9].green();
  gl[10] = pixels[10].green();  gl[11] = pixels[11].green();
  gl[12] = pixels[12].green();  gl[13] = pixels[13].green();
  gl[14] = pixels[14].green();  gl[15] = pixels[15].green();

  bl[0] = pixels[0].blue();    bl[1] = pixels[1].blue();
  bl[2] = pixels[2].blue();    bl[3] = pixels[3].blue();
  bl[4] = pixels[4].blue();    bl[5] = pixels[5].blue();
  bl[6] = pixels[6].blue();    bl[7] = pixels[7].blue();
  bl[8] = pixels[8].blue();    bl[9] = pixels[9].blue();
  bl[10] = pixels[10].blue();  bl[11] = pixels[11].blue();
  bl[12] = pixels[12].blue();  bl[13] = pixels[13].blue();
  bl[14] = pixels[14].blue();  bl[15] = pixels[15].blue();

  al[0] = pixels[0].alpha();    al[1] = pixels[1].alpha();
  al[2] = pixels[2].alpha();    al[3] = pixels[3].alpha();
  al[4] = pixels[4].alpha();    al[5] = pixels[5].alpha();
  al[6] = pixels[6].alpha();    al[7] = pixels[7].alpha();
  al[8] = pixels[8].alpha();    al[9] = pixels[9].alpha();
  al[10] = pixels[10].alpha();  al[11] = pixels[11].alpha();
  al[12] = pixels[12].alpha();  al[13] = pixels[13].alpha();
  al[14] = pixels[14].alpha();  al[15] = pixels[15].alpha();
}

void load_8888_tail(std::size_t count,
                    std::span<const PremultipliedColorU8> pixels,
                    U16x16T& or_,
                    U16x16T& og,
                    U16x16T& ob,
                    U16x16T& oa) {
  std::array<PremultipliedColorU8, kStageWidth> tmp{};
  tmp.fill(PremultipliedColorU8::transparent);
  std::copy_n(pixels.begin(), count, tmp.begin());
  load_8888_lowp(tmp, or_, og, ob, oa);
}

// Matches Rust store_8888(&u16x16, ..., &mut [PremultipliedColorU8; STAGE_WIDTH]).
// Stores U16x16T pixel channels to u8 pixels.
void store_8888_lowp(std::span<PremultipliedColorU8> pixels,
                     const U16x16T& r,
                     const U16x16T& g,
                     const U16x16T& b,
                     const U16x16T& a) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  if constexpr (useAarch64NeonNative()) {
    static_assert(sizeof(PremultipliedColorU8) == 4);

    const uint16x8x2_t r16 = vld1q_u16_x2(r.lanes().data());
    const uint16x8x2_t g16 = vld1q_u16_x2(g.lanes().data());
    const uint16x8x2_t b16 = vld1q_u16_x2(b.lanes().data());
    const uint16x8x2_t a16 = vld1q_u16_x2(a.lanes().data());

    uint8x16x4_t rgba{};
    // Rust/scalar lowp store uses u16->u8 truncation (wrap), not saturation.
    rgba.val[0] = vcombine_u8(vmovn_u16(r16.val[0]), vmovn_u16(r16.val[1]));
    rgba.val[1] = vcombine_u8(vmovn_u16(g16.val[0]), vmovn_u16(g16.val[1]));
    rgba.val[2] = vcombine_u8(vmovn_u16(b16.val[0]), vmovn_u16(b16.val[1]));
    rgba.val[3] = vcombine_u8(vmovn_u16(a16.val[0]), vmovn_u16(a16.val[1]));

    auto* packed = reinterpret_cast<std::uint8_t*>(pixels.data());
    vst4q_u8(packed, rgba);
    return;
  }
#endif

#if defined(__x86_64__) || defined(__i386__)
  if constexpr (useX86Avx2FmaNative()) {
    static_assert(sizeof(PremultipliedColorU8) == 4);

    const __m128i byte_mask = _mm_set1_epi16(0x00FF);
    const __m128i r_lo = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(r.lanes().data())), byte_mask);
    const __m128i r_hi = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(r.lanes().data() + 8)), byte_mask);
    const __m128i g_lo = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.lanes().data())), byte_mask);
    const __m128i g_hi = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.lanes().data() + 8)), byte_mask);
    const __m128i b_lo = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(b.lanes().data())), byte_mask);
    const __m128i b_hi = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(b.lanes().data() + 8)), byte_mask);
    const __m128i a_lo = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(a.lanes().data())), byte_mask);
    const __m128i a_hi = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(a.lanes().data() + 8)), byte_mask);

    const __m128i r8 = _mm_packus_epi16(r_lo, r_hi);
    const __m128i g8 = _mm_packus_epi16(g_lo, g_hi);
    const __m128i b8 = _mm_packus_epi16(b_lo, b_hi);
    const __m128i a8 = _mm_packus_epi16(a_lo, a_hi);

    const __m128i rg_lo = _mm_unpacklo_epi8(r8, g8);
    const __m128i rg_hi = _mm_unpackhi_epi8(r8, g8);
    const __m128i ba_lo = _mm_unpacklo_epi8(b8, a8);
    const __m128i ba_hi = _mm_unpackhi_epi8(b8, a8);

    const __m128i rgba0 = _mm_unpacklo_epi16(rg_lo, ba_lo);
    const __m128i rgba1 = _mm_unpackhi_epi16(rg_lo, ba_lo);
    const __m128i rgba2 = _mm_unpacklo_epi16(rg_hi, ba_hi);
    const __m128i rgba3 = _mm_unpackhi_epi16(rg_hi, ba_hi);

    auto* packed = reinterpret_cast<std::uint8_t*>(pixels.data());
    _mm_storeu_si128(reinterpret_cast<__m128i*>(packed + 0), rgba0);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(packed + 16), rgba1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(packed + 32), rgba2);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(packed + 48), rgba3);
    return;
  }
#endif

  const auto& rl = r.lanes();
  const auto& gl = g.lanes();
  const auto& bl = b.lanes();
  const auto& al = a.lanes();

  pixels[0] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[0]), static_cast<std::uint8_t>(gl[0]),
      static_cast<std::uint8_t>(bl[0]), static_cast<std::uint8_t>(al[0]));
  pixels[1] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[1]), static_cast<std::uint8_t>(gl[1]),
      static_cast<std::uint8_t>(bl[1]), static_cast<std::uint8_t>(al[1]));
  pixels[2] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[2]), static_cast<std::uint8_t>(gl[2]),
      static_cast<std::uint8_t>(bl[2]), static_cast<std::uint8_t>(al[2]));
  pixels[3] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[3]), static_cast<std::uint8_t>(gl[3]),
      static_cast<std::uint8_t>(bl[3]), static_cast<std::uint8_t>(al[3]));
  pixels[4] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[4]), static_cast<std::uint8_t>(gl[4]),
      static_cast<std::uint8_t>(bl[4]), static_cast<std::uint8_t>(al[4]));
  pixels[5] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[5]), static_cast<std::uint8_t>(gl[5]),
      static_cast<std::uint8_t>(bl[5]), static_cast<std::uint8_t>(al[5]));
  pixels[6] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[6]), static_cast<std::uint8_t>(gl[6]),
      static_cast<std::uint8_t>(bl[6]), static_cast<std::uint8_t>(al[6]));
  pixels[7] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[7]), static_cast<std::uint8_t>(gl[7]),
      static_cast<std::uint8_t>(bl[7]), static_cast<std::uint8_t>(al[7]));
  pixels[8] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[8]), static_cast<std::uint8_t>(gl[8]),
      static_cast<std::uint8_t>(bl[8]), static_cast<std::uint8_t>(al[8]));
  pixels[9] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[9]), static_cast<std::uint8_t>(gl[9]),
      static_cast<std::uint8_t>(bl[9]), static_cast<std::uint8_t>(al[9]));
  pixels[10] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[10]), static_cast<std::uint8_t>(gl[10]),
      static_cast<std::uint8_t>(bl[10]), static_cast<std::uint8_t>(al[10]));
  pixels[11] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[11]), static_cast<std::uint8_t>(gl[11]),
      static_cast<std::uint8_t>(bl[11]), static_cast<std::uint8_t>(al[11]));
  pixels[12] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[12]), static_cast<std::uint8_t>(gl[12]),
      static_cast<std::uint8_t>(bl[12]), static_cast<std::uint8_t>(al[12]));
  pixels[13] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[13]), static_cast<std::uint8_t>(gl[13]),
      static_cast<std::uint8_t>(bl[13]), static_cast<std::uint8_t>(al[13]));
  pixels[14] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[14]), static_cast<std::uint8_t>(gl[14]),
      static_cast<std::uint8_t>(bl[14]), static_cast<std::uint8_t>(al[14]));
  pixels[15] = PremultipliedColorU8::fromRgbaUnchecked(
      static_cast<std::uint8_t>(rl[15]), static_cast<std::uint8_t>(gl[15]),
      static_cast<std::uint8_t>(bl[15]), static_cast<std::uint8_t>(al[15]));
}

void store_8888_tail(std::size_t count,
                     std::span<PremultipliedColorU8> pixels,
                     const U16x16T& r,
                     const U16x16T& g,
                     const U16x16T& b,
                     const U16x16T& a) {
  const auto& rl = r.lanes();
  const auto& gl = g.lanes();
  const auto& bl = b.lanes();
  const auto& al = a.lanes();
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pixels[i] = PremultipliedColorU8::fromRgbaUnchecked(
        static_cast<std::uint8_t>(rl[i]),
        static_cast<std::uint8_t>(gl[i]),
        static_cast<std::uint8_t>(bl[i]),
        static_cast<std::uint8_t>(al[i]));
    if (i + 1 == count) {
      break;
    }
  }
}

void load_8_lowp(std::span<const std::uint8_t> data, U16x16T& a) {
#if defined(__aarch64__) && defined(__ARM_NEON)
  if constexpr (useAarch64NeonNative()) {
    const uint8x16_t src = vld1q_u8(data.data());
    const uint16x8x2_t wide = {vmovl_u8(vget_low_u8(src)),
                               vmovl_u8(vget_high_u8(src))};
    vst1q_u16_x2(a.lanes().data(), wide);
    return;
  }
#endif

#if defined(__x86_64__) || defined(__i386__)
  if constexpr (useX86Avx2FmaNative()) {
    const __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data.data()));
    const __m128i zero = _mm_setzero_si128();
    _mm_storeu_si128(reinterpret_cast<__m128i*>(a.lanes().data()),
                     _mm_unpacklo_epi8(src, zero));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(a.lanes().data() + 8),
                     _mm_unpackhi_epi8(src, zero));
    return;
  }
#endif

  auto& al = a.lanes();
  al[0] = data[0];    al[1] = data[1];
  al[2] = data[2];    al[3] = data[3];
  al[4] = data[4];    al[5] = data[5];
  al[6] = data[6];    al[7] = data[7];
  al[8] = data[8];    al[9] = data[9];
  al[10] = data[10];  al[11] = data[11];
  al[12] = data[12];  al[13] = data[13];
  al[14] = data[14];  al[15] = data[15];
}

void load_8_tail(std::size_t count, std::span<const std::uint8_t> data, U16x16T& a) {
  std::array<std::uint8_t, kStageWidth> tmp{};
  std::copy_n(data.begin(), count, tmp.begin());
  load_8_lowp(tmp, a);
}

void load_dst(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_lowp(pixels, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.nextStage();
}

void load_dst_tail(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_tail(pipeline.tail, pixels, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.nextStage();
}

void store(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  store_8888_lowp(pixels, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void store_tail(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  store_8888_tail(pipeline.tail, pixels, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void load_dst_u8(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  load_8_lowp(std::span<const std::uint8_t>(pipeline.pixmap_dst->data + offset, kStageWidth),
              pipeline.da);
  pipeline.nextStage();
}

void load_dst_u8_tail(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  load_8_tail(pipeline.tail,
              std::span<const std::uint8_t>(pipeline.pixmap_dst->data + offset, pipeline.tail),
              pipeline.da);
  pipeline.nextStage();
}

void store_u8(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  const auto& al = pipeline.a.lanes();
#if defined(__aarch64__) && defined(__ARM_NEON)
  if constexpr (useAarch64NeonNative()) {
    const uint16x8x2_t a16 = vld1q_u16_x2(al.data());
    const uint8x16_t a8 = vcombine_u8(vmovn_u16(a16.val[0]), vmovn_u16(a16.val[1]));
    vst1q_u8(pipeline.pixmap_dst->data + offset, a8);
    pipeline.nextStage();
    return;
  }
#endif

#if defined(__x86_64__) || defined(__i386__)
  if constexpr (useX86Avx2FmaNative()) {
    const __m128i byte_mask = _mm_set1_epi16(0x00FF);
    const __m128i a_lo = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(al.data())), byte_mask);
    const __m128i a_hi = _mm_and_si128(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(al.data() + 8)), byte_mask);
    const __m128i a8 = _mm_packus_epi16(a_lo, a_hi);
    _mm_storeu_si128(
        reinterpret_cast<__m128i*>(pipeline.pixmap_dst->data + offset), a8);
    pipeline.nextStage();
    return;
  }
#endif

  pipeline.pixmap_dst->data[offset + 0] = static_cast<std::uint8_t>(al[0]);
  pipeline.pixmap_dst->data[offset + 1] = static_cast<std::uint8_t>(al[1]);
  pipeline.pixmap_dst->data[offset + 2] = static_cast<std::uint8_t>(al[2]);
  pipeline.pixmap_dst->data[offset + 3] = static_cast<std::uint8_t>(al[3]);
  pipeline.pixmap_dst->data[offset + 4] = static_cast<std::uint8_t>(al[4]);
  pipeline.pixmap_dst->data[offset + 5] = static_cast<std::uint8_t>(al[5]);
  pipeline.pixmap_dst->data[offset + 6] = static_cast<std::uint8_t>(al[6]);
  pipeline.pixmap_dst->data[offset + 7] = static_cast<std::uint8_t>(al[7]);
  pipeline.pixmap_dst->data[offset + 8] = static_cast<std::uint8_t>(al[8]);
  pipeline.pixmap_dst->data[offset + 9] = static_cast<std::uint8_t>(al[9]);
  pipeline.pixmap_dst->data[offset + 10] = static_cast<std::uint8_t>(al[10]);
  pipeline.pixmap_dst->data[offset + 11] = static_cast<std::uint8_t>(al[11]);
  pipeline.pixmap_dst->data[offset + 12] = static_cast<std::uint8_t>(al[12]);
  pipeline.pixmap_dst->data[offset + 13] = static_cast<std::uint8_t>(al[13]);
  pipeline.pixmap_dst->data[offset + 14] = static_cast<std::uint8_t>(al[14]);
  pipeline.pixmap_dst->data[offset + 15] = static_cast<std::uint8_t>(al[15]);
  pipeline.nextStage();
}

void store_u8_tail(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  const auto& al = pipeline.a.lanes();
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.pixmap_dst->data[offset + i] = static_cast<std::uint8_t>(al[i]);
    if (i + 1 == pipeline.tail) {
      break;
    }
  }
  pipeline.nextStage();
}

void load_mask_u8(Pipeline& pipeline) {
  if (pipeline.mask_ctx == nullptr || pipeline.mask_ctx->data == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.mask_ctx->byteOffset(pipeline.dx, pipeline.dy);
  auto& al = pipeline.a.lanes();
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    al[i] = static_cast<std::uint16_t>(pipeline.mask_ctx->data[offset + i]);
  }
  for (std::size_t i = pipeline.tail; i < kStageWidth; ++i) {
    al[i] = 0;
  }
  pipeline.r = U16x16T::splat(0);
  pipeline.g = U16x16T::splat(0);
  pipeline.b = U16x16T::splat(0);
  pipeline.nextStage();
}

void mask_u8(Pipeline& pipeline) {
  if (pipeline.mask_ctx == nullptr || pipeline.mask_ctx->data == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.mask_ctx->byteOffset(pipeline.dx, pipeline.dy);
  U16x16T c{};
  auto& cl = c.lanes();
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    cl[i] = static_cast<std::uint16_t>(pipeline.mask_ctx->data[offset + i]);
  }
  bool all_zero = true;
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    if (cl[i] != 0) {
      all_zero = false;
      break;
    }
  }
  if (all_zero) {
    return;
  }
  pipeline.r = mulDiv255(pipeline.r, c);
  pipeline.g = mulDiv255(pipeline.g, c);
  pipeline.b = mulDiv255(pipeline.b, c);
  pipeline.a = mulDiv255(pipeline.a, c);
  pipeline.nextStage();
}

void source_over_rgba(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_lowp(pixels, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.r = sourceOverChannel(pipeline.r, pipeline.dr, pipeline.a);
  pipeline.g = sourceOverChannel(pipeline.g, pipeline.dg, pipeline.a);
  pipeline.b = sourceOverChannel(pipeline.b, pipeline.db, pipeline.a);
  pipeline.a = sourceOverChannel(pipeline.a, pipeline.da, pipeline.a);
  store_8888_lowp(pixels, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void source_over_rgba_tail(Pipeline& pipeline) {
  assert(pipeline.pixmap_dst != nullptr);
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_tail(pipeline.tail, pixels, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.r = sourceOverChannel(pipeline.r, pipeline.dr, pipeline.a);
  pipeline.g = sourceOverChannel(pipeline.g, pipeline.dg, pipeline.a);
  pipeline.b = sourceOverChannel(pipeline.b, pipeline.db, pipeline.a);
  pipeline.a = sourceOverChannel(pipeline.a, pipeline.da, pipeline.a);
  store_8888_tail(pipeline.tail, pixels, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void darken(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return s + d - div255((s * da).max(d * sa));
  });
}

void lighten(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return s + d - div255((s * da).min(d * sa));
  });
}

void difference(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return s + d - U16x16T::splat(2) * div255((s * da).min(d * sa));
  });
}

void exclusion(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T /*da*/) {
    return s + d - U16x16T::splat(2) * div255(s * d);
  });
}

void hard_light(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    const auto inv_da = U16x16T::splat(255) - da;
    const auto inv_sa = U16x16T::splat(255) - sa;
    const auto body_if = U16x16T::splat(2) * s * d;
    const auto body_else = sa * da - U16x16T::splat(2) * (sa - s) * (da - d);
    const auto mask = (U16x16T::splat(2) * s).cmpLe(sa);
    const auto body = mask.blend(body_if, body_else);
    return div255(s * inv_da + d * inv_sa + body);
  });
}

void overlay(Pipeline& pipeline) {
  blend_fn2(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    const auto inv_da = U16x16T::splat(255) - da;
    const auto inv_sa = U16x16T::splat(255) - sa;
    const auto body_if = U16x16T::splat(2) * s * d;
    const auto body_else = sa * da - U16x16T::splat(2) * (sa - s) * (da - d);
    const auto mask = (U16x16T::splat(2) * d).cmpLe(da);
    const auto body = mask.blend(body_if, body_else);
    return div255(s * inv_da + d * inv_sa + body);
  });
}

// --- Lowp coordinate/shader stages ---
//
// Rust lowp uses split()/join() to convert between f32x16 (a single 512-bit
// float SIMD vector used for coordinates) and pairs of u16x16 (256-bit integer
// SIMD vectors used for pixel channels). The pipeline stores coordinates by
// splitting one f32x16 across two u16x16 registers (x -> r,g and y -> b,a),
// then join() reconstructs the f32x16 before coordinate math.

void transform(Pipeline& pipeline) {
  const auto& ts = pipeline.ctx->transform;
  auto x = join(pipeline.r, pipeline.g);
  auto y = join(pipeline.b, pipeline.a);
  // Match Rust's nested mad: x * sx + (y * kx + tx)
  auto nx = x * F32x16T::splat(ts.sx) + (y * F32x16T::splat(ts.kx) + F32x16T::splat(ts.tx));
  auto ny = x * F32x16T::splat(ts.ky) + (y * F32x16T::splat(ts.sy) + F32x16T::splat(ts.ty));
  split(nx, pipeline.r, pipeline.g);
  split(ny, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void pad_x1(Pipeline& pipeline) {
  auto x = join(pipeline.r, pipeline.g);
  split(x.normalize(), pipeline.r, pipeline.g);
  pipeline.nextStage();
}

void reflect_x1(Pipeline& pipeline) {
  auto x = join(pipeline.r, pipeline.g);
  auto v_minus_1 = x - F32x16T::splat(1.0f);
  auto result = (v_minus_1 - F32x16T::splat(2.0f) * (v_minus_1 * F32x16T::splat(0.5f)).floor()
                 - F32x16T::splat(1.0f)).abs().normalize();
  split(result, pipeline.r, pipeline.g);
  pipeline.nextStage();
}

void repeat_x1(Pipeline& pipeline) {
  auto x = join(pipeline.r, pipeline.g);
  auto result = (x - x.floor()).normalize();
  split(result, pipeline.r, pipeline.g);
  pipeline.nextStage();
}

void evenly_spaced_2_stop_gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->evenly_spaced_2_stop_gradient;
  auto t = join(pipeline.r, pipeline.g);
  auto rf = t * F32x16T::splat(ctx.factor.r) + F32x16T::splat(ctx.bias.r);
  auto gf = t * F32x16T::splat(ctx.factor.g) + F32x16T::splat(ctx.bias.g);
  auto bf = t * F32x16T::splat(ctx.factor.b) + F32x16T::splat(ctx.bias.b);
  auto af = t * F32x16T::splat(ctx.factor.a) + F32x16T::splat(ctx.bias.a);
  roundF32ToU16(rf, gf, bf, af, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->gradient;
  auto t = join(pipeline.r, pipeline.g);

  // Per-lane index lookup — inherently scalar
  auto t_lo = t.lo().lanes();
  auto t_hi = t.hi().lanes();

  std::array<float, 8> r_lo{}, g_lo{}, b_lo{}, a_lo{};
  std::array<float, 8> r_hi{}, g_hi{}, b_hi{}, a_hi{};

  auto process = [&](const std::array<float, 8>& tv,
                     std::array<float, 8>& rv, std::array<float, 8>& gv,
                     std::array<float, 8>& bv, std::array<float, 8>& av) {
    for (std::size_t i = 0; i < 8; ++i) {
      std::uint32_t idx = 0;
      for (std::size_t s = 1; s < ctx.len; ++s) {
        if (tv[i] >= ctx.t_values[s]) {
          idx += 1;
        }
      }
      const auto& factor = ctx.factors[idx];
      const auto& bias = ctx.biases[idx];
      rv[i] = tv[i] * factor.r + bias.r;
      gv[i] = tv[i] * factor.g + bias.g;
      bv[i] = tv[i] * factor.b + bias.b;
      av[i] = tv[i] * factor.a + bias.a;
    }
  };

  process(t_lo, r_lo, g_lo, b_lo, a_lo);
  process(t_hi, r_hi, g_hi, b_hi, a_hi);

  auto rf = F32x16T(F32x8T(r_lo), F32x8T(r_hi));
  auto gf = F32x16T(F32x8T(g_lo), F32x8T(g_hi));
  auto bf = F32x16T(F32x8T(b_lo), F32x8T(b_hi));
  auto af = F32x16T(F32x8T(a_lo), F32x8T(a_hi));

  roundF32ToU16(rf, gf, bf, af, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void xy_to_radius(Pipeline& pipeline) {
  auto x = join(pipeline.r, pipeline.g);
  auto y = join(pipeline.b, pipeline.a);
  auto radius = (x * x + y * y).sqrt();
  split(radius, pipeline.r, pipeline.g);
  pipeline.nextStage();
}

}  // namespace

void justReturn(Pipeline& pipeline) {
  (void)pipeline;
}

void start(const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& functions,
           const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& tail_functions,
           const ScreenIntRect& rect,
           const AAMaskCtx& aa_mask_ctx,
           const MaskCtx& mask_ctx,
           Context& ctx,
           SubPixmapMut* pixmap_dst) {
  Pipeline p(functions, tail_functions, rect, aa_mask_ctx, mask_ctx, ctx, pixmap_dst);

  for (std::size_t y = rect.y(); y < rect.bottom(); ++y) {
    std::size_t x = rect.x();
    const std::size_t end = rect.right();

    p.functions = &functions;
    while (x + kStageWidth <= end) {
      p.index = 0;
      p.dx = x;
      p.dy = y;
      p.tail = kStageWidth;
      p.nextStage();
      x += kStageWidth;
    }

    if (x != end) {
      p.index = 0;
      p.functions = &tail_functions;
      p.dx = x;
      p.dy = y;
      p.tail = end - x;
      p.nextStage();
    }
  }
}

const std::array<StageFn, kStagesCount> STAGES = {
    move_source_to_destination,
    move_destination_to_source,
    null_fn,
    null_fn,
    premultiply,
    uniform_color,
    seed_shader,
    load_dst,
    store,
    load_dst_u8,
    store_u8,
    null_fn,
    load_mask_u8,
    mask_u8,
    scale_u8,
    lerp_u8,
    scale_1_float,
    lerp_1_float,
    destination_atop,
    destination_in,
    destination_out,
    destination_over,
    source_atop,
    source_in,
    source_out,
    source_over,
    clear,
    modulate,
    multiply,
    plus,
    screen,
    x_or,
    null_fn,
    null_fn,
    darken,
    difference,
    exclusion,
    hard_light,
    lighten,
    overlay,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,  // Luminosity (not lowp compatible)
    source_over_rgba,
    transform,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    pad_x1,
    reflect_x1,
    repeat_x1,
    gradient,
    evenly_spaced_2_stop_gradient,
    null_fn,
    xy_to_radius,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
    null_fn,
};

const std::array<StageFn, kStagesCount> STAGES_TAIL = [] {
  auto stages = STAGES;
  stages[static_cast<std::size_t>(Stage::LoadDestination)] = load_dst_tail;
  stages[static_cast<std::size_t>(Stage::Store)] = store_tail;
  stages[static_cast<std::size_t>(Stage::LoadDestinationU8)] = load_dst_u8_tail;
  stages[static_cast<std::size_t>(Stage::StoreU8)] = store_u8_tail;
  stages[static_cast<std::size_t>(Stage::SourceOverRgba)] = source_over_rgba_tail;
  return stages;
}();

}  // namespace tiny_skia::pipeline::lowp
