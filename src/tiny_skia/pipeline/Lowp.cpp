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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>

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
      : functions(&fun),
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
  return (v + U16x16T::splat(255)) >> U16x16T::splat(8);
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
  pipeline.r = div255(pipeline.r * pipeline.a);
  pipeline.g = div255(pipeline.g * pipeline.a);
  pipeline.b = div255(pipeline.b * pipeline.a);
  pipeline.nextStage();
}

void uniform_color(Pipeline& pipeline) {
  const auto& u = pipeline.ctx->uniform_color;
  pipeline.r = U16x16T::splat(u.rgba[0]);
  pipeline.g = U16x16T::splat(u.rgba[1]);
  pipeline.b = U16x16T::splat(u.rgba[2]);
  pipeline.a = U16x16T::splat(u.rgba[3]);
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
  pipeline.r = div255(pipeline.r * c);
  pipeline.g = div255(pipeline.g * c);
  pipeline.b = div255(pipeline.b * c);
  pipeline.a = div255(pipeline.a * c);
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
  pipeline.r = div255(pipeline.dr * inv_c + pipeline.r * c);
  pipeline.g = div255(pipeline.dg * inv_c + pipeline.g * c);
  pipeline.b = div255(pipeline.db * inv_c + pipeline.b * c);
  pipeline.a = div255(pipeline.da * inv_c + pipeline.a * c);
  pipeline.nextStage();
}

void scale_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  pipeline.r = div255(pipeline.r * c);
  pipeline.g = div255(pipeline.g * c);
  pipeline.b = div255(pipeline.b * c);
  pipeline.a = div255(pipeline.a * c);
  pipeline.nextStage();
}

void lerp_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  const auto inv_c = U16x16T::splat(255) - c;
  pipeline.r = div255(pipeline.dr * inv_c + pipeline.r * c);
  pipeline.g = div255(pipeline.dg * inv_c + pipeline.g * c);
  pipeline.b = div255(pipeline.db * inv_c + pipeline.b * c);
  pipeline.a = div255(pipeline.da * inv_c + pipeline.a * c);
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
  pipeline.a = sa + div255(da * (U16x16T::splat(255) - sa));
  pipeline.nextStage();
}

void clear(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T /*d*/, U16x16T /*sa*/, U16x16T /*da*/) {
    return U16x16T::splat(0);
  });
}

void destination_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return div255(d * sa + s * (U16x16T::splat(255) - da));
  });
}

void destination_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return div255(d * sa);
  });
}

void destination_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T /*s*/, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return div255(d * (U16x16T::splat(255) - sa));
  });
}

void source_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return div255(s * da + d * (U16x16T::splat(255) - sa));
  });
}

void source_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T /*d*/, U16x16T /*sa*/, U16x16T da) {
    return div255(s * da);
  });
}

void source_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T /*d*/, U16x16T /*sa*/, U16x16T da) {
    return div255(s * (U16x16T::splat(255) - da));
  });
}

void source_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T /*da*/) {
    return s + div255(d * (U16x16T::splat(255) - sa));
  });
}

void destination_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T da) {
    return d + div255(s * (U16x16T::splat(255) - da));
  });
}

void modulate(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T /*sa*/, U16x16T /*da*/) {
    return div255(s * d);
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
    return s + d - div255(s * d);
  });
}

void x_or(Pipeline& pipeline) {
  blend_fn(pipeline, [](U16x16T s, U16x16T d, U16x16T sa, U16x16T da) {
    return div255(s * (U16x16T::splat(255) - da) + d * (U16x16T::splat(255) - sa));
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
// Loads u8 pixel channels into U16x16T (zero-extend u8 → u16).
void load_8888_lowp(std::span<const PremultipliedColorU8> pixels,
                    std::size_t count,
                    U16x16T& or_, U16x16T& og,
                    U16x16T& ob, U16x16T& oa) {
  auto& rl = or_.lanes(); auto& gl = og.lanes();
  auto& bl = ob.lanes(); auto& al = oa.lanes();
  for (std::size_t i = 0; i < count; ++i) {
    rl[i] = pixels[i].red();
    gl[i] = pixels[i].green();
    bl[i] = pixels[i].blue();
    al[i] = pixels[i].alpha();
  }
  for (std::size_t i = count; i < kStageWidth; ++i) {
    rl[i] = 0; gl[i] = 0; bl[i] = 0; al[i] = 0;
  }
}

// Matches Rust store_8888(&u16x16, ..., &mut [PremultipliedColorU8; STAGE_WIDTH]).
// Stores U16x16T pixel channels to u8 pixels (clamp+truncate).
void store_8888_lowp(std::span<PremultipliedColorU8> pixels,
                     std::size_t count,
                     const U16x16T& r,
                     const U16x16T& g,
                     const U16x16T& b,
                     const U16x16T& a) {
  const auto& rl = r.lanes(); const auto& gl = g.lanes();
  const auto& bl = b.lanes(); const auto& al = a.lanes();
  for (std::size_t i = 0; i < count; ++i) {
    pixels[i] = PremultipliedColorU8::fromRgbaUnchecked(
        static_cast<std::uint8_t>(std::min<std::uint16_t>(rl[i], 255)),
        static_cast<std::uint8_t>(std::min<std::uint16_t>(gl[i], 255)),
        static_cast<std::uint8_t>(std::min<std::uint16_t>(bl[i], 255)),
        static_cast<std::uint8_t>(std::min<std::uint16_t>(al[i], 255)));
  }
}

void load_dst(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_lowp(pixels, pipeline.tail,
                 pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.nextStage();
}

void store(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  store_8888_lowp(pixels, pipeline.tail,
                  pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void load_dst_u8(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  auto& dal = pipeline.da.lanes();
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    dal[i] = static_cast<std::uint16_t>(pipeline.pixmap_dst->data[offset + i]);
  }
  for (std::size_t i = pipeline.tail; i < kStageWidth; ++i) {
    dal[i] = 0;
  }
  pipeline.nextStage();
}

void store_u8(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  const auto& al = pipeline.a.lanes();
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    pipeline.pixmap_dst->data[offset + i] = static_cast<std::uint8_t>(al[i]);
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
  pipeline.r = div255(pipeline.r * c);
  pipeline.g = div255(pipeline.g * c);
  pipeline.b = div255(pipeline.b * c);
  pipeline.a = div255(pipeline.a * c);
  pipeline.nextStage();
}

void source_over_rgba(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  auto pixels = pixelsAtXY(*pipeline.pixmap_dst, pipeline.dx, pipeline.dy);
  load_8888_lowp(pixels, pipeline.tail,
                 pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  const auto inv_sa = U16x16T::splat(255) - pipeline.a;
  pipeline.r = pipeline.r + div255(pipeline.dr * inv_sa);
  pipeline.g = pipeline.g + div255(pipeline.dg * inv_sa);
  pipeline.b = pipeline.b + div255(pipeline.db * inv_sa);
  pipeline.a = pipeline.a + div255(pipeline.da * inv_sa);
  store_8888_lowp(pixels, pipeline.tail,
                  pipeline.r, pipeline.g, pipeline.b, pipeline.a);
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

}  // namespace tiny_skia::pipeline::lowp
