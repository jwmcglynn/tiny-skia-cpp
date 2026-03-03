// Disable FMA contraction to match Rust's lowp pipeline, which uses software
// SIMD wrappers (f32x16/f32x8) that prevent LLVM from fusing multiply-add.
#pragma clang fp contract(off)

#include "tiny_skia/pipeline/Lowp.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Pixmap.h"

#include <algorithm>
#include <cmath>
#include <span>

namespace tiny_skia::pipeline::lowp {

// Maps to Rust's u16x16 — a 16-element channel of lowp pixel values in [0, 255].
// C++ uses float rather than u16, but the value range and lane count are identical.
using LowpChannel = std::array<float, kStageWidth>;

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

// Matches Rust lowp: (v + 255) >> 8
// Truncates to integer to avoid float fraction accumulation across stages.
inline float div255(float v) {
  // Match Rust lowp: (v + 255) >> 8, i.e. integer floor((v + 255) / 256).
  return std::floor((v + 255.0f) * (1.0f / 256.0f));
}

// Matches Rust lowp: from_float(f) = (f * 255.0 + 0.5) as u16
inline float fromFloat(float f) {
  return std::floor(f * 255.0f + 0.5f);
}

// Matches Rust normalize(): clamp to [0, 1]
inline float normalize(float f) {
  return std::max(0.0f, std::min(1.0f, f));
}

void move_source_to_destination(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.dr[i] = pipeline.r[i];
    pipeline.dg[i] = pipeline.g[i];
    pipeline.db[i] = pipeline.b[i];
    pipeline.da[i] = pipeline.a[i];
  }
  pipeline.nextStage();
}

void move_destination_to_source(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.dr[i];
    pipeline.g[i] = pipeline.dg[i];
    pipeline.b[i] = pipeline.db[i];
    pipeline.a[i] = pipeline.da[i];
  }
  pipeline.nextStage();
}

void premultiply(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = div255(pipeline.r[i] * pipeline.a[i]);
    pipeline.g[i] = div255(pipeline.g[i] * pipeline.a[i]);
    pipeline.b[i] = div255(pipeline.b[i] * pipeline.a[i]);
  }
  pipeline.nextStage();
}

void uniform_color(Pipeline& pipeline) {
  const auto& u = pipeline.ctx->uniform_color;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = static_cast<float>(u.rgba[0]);
    pipeline.g[i] = static_cast<float>(u.rgba[1]);
    pipeline.b[i] = static_cast<float>(u.rgba[2]);
    pipeline.a[i] = static_cast<float>(u.rgba[3]);
  }
  pipeline.nextStage();
}

void seed_shader(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.dr[i] = 0.0f;
    pipeline.dg[i] = 0.0f;
    pipeline.db[i] = 0.0f;
    pipeline.da[i] = 0.0f;
    pipeline.r[i] = static_cast<float>(pipeline.dx) + static_cast<float>(i) + 0.5f;
    pipeline.g[i] = static_cast<float>(pipeline.dy) + 0.5f;
    pipeline.b[i] = 1.0f;
    pipeline.a[i] = 0.0f;
  }
  pipeline.nextStage();
}

void scale_u8(Pipeline& pipeline) {
  const auto data = pipeline.aa_mask_ctx->copyAtXY(pipeline.dx, pipeline.dy, pipeline.tail);
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto c = static_cast<float>(i < 2 ? data[i] : 0u);
    pipeline.r[i] = div255(pipeline.r[i] * c);
    pipeline.g[i] = div255(pipeline.g[i] * c);
    pipeline.b[i] = div255(pipeline.b[i] * c);
    pipeline.a[i] = div255(pipeline.a[i] * c);
  }
  pipeline.nextStage();
}

void lerp_u8(Pipeline& pipeline) {
  const auto data = pipeline.aa_mask_ctx->copyAtXY(pipeline.dx, pipeline.dy, pipeline.tail);
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto c = static_cast<float>(i < 2 ? data[i] : 0u);
    pipeline.r[i] = div255(pipeline.dr[i] * (255.0f - c) + pipeline.r[i] * c);
    pipeline.g[i] = div255(pipeline.dg[i] * (255.0f - c) + pipeline.g[i] * c);
    pipeline.b[i] = div255(pipeline.db[i] * (255.0f - c) + pipeline.b[i] * c);
    pipeline.a[i] = div255(pipeline.da[i] * (255.0f - c) + pipeline.a[i] * c);
  }
  pipeline.nextStage();
}

void scale_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = div255(pipeline.r[i] * c);
    pipeline.g[i] = div255(pipeline.g[i] * c);
    pipeline.b[i] = div255(pipeline.b[i] * c);
    pipeline.a[i] = div255(pipeline.a[i] * c);
  }
  pipeline.nextStage();
}

void lerp_1_float(Pipeline& pipeline) {
  const auto c = fromFloat(pipeline.ctx->current_coverage);
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = div255(pipeline.dr[i] * (255.0f - c) + pipeline.r[i] * c);
    pipeline.g[i] = div255(pipeline.dg[i] * (255.0f - c) + pipeline.g[i] * c);
    pipeline.b[i] = div255(pipeline.db[i] * (255.0f - c) + pipeline.b[i] * c);
    pipeline.a[i] = div255(pipeline.da[i] * (255.0f - c) + pipeline.a[i] * c);
  }
  pipeline.nextStage();
}

// --- Blend-mode helpers (mirrors Rust's blend_fn! / blend_fn2! macros) ---
//
// blend_fn:  applies F(s, d, sa, da) uniformly to all four channels (r, g, b, a).
// blend_fn2: applies F(s, d, sa, da) to r, g, b only; alpha uses source_over:
//            a' = sa + div255(da * (255 - sa)).

template <typename F>
void blend_fn(Pipeline& pipeline, F&& f) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sa = pipeline.a[i], da = pipeline.da[i];
    pipeline.r[i] = f(pipeline.r[i], pipeline.dr[i], sa, da);
    pipeline.g[i] = f(pipeline.g[i], pipeline.dg[i], sa, da);
    pipeline.b[i] = f(pipeline.b[i], pipeline.db[i], sa, da);
    pipeline.a[i] = f(sa, da, sa, da);
  }
  pipeline.nextStage();
}

template <typename F>
void blend_fn2(Pipeline& pipeline, F&& f) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sa = pipeline.a[i], da = pipeline.da[i];
    pipeline.r[i] = f(pipeline.r[i], pipeline.dr[i], sa, da);
    pipeline.g[i] = f(pipeline.g[i], pipeline.dg[i], sa, da);
    pipeline.b[i] = f(pipeline.b[i], pipeline.db[i], sa, da);
    pipeline.a[i] = sa + div255(da * (255.0f - sa));
  }
  pipeline.nextStage();
}

void clear(Pipeline& pipeline) {
  blend_fn(pipeline, [](float /*s*/, float /*d*/, float /*sa*/, float /*da*/) {
    return 0.0f;
  });
}

void destination_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float sa, float da) {
    return div255(d * sa + s * (255.0f - da));
  });
}

void destination_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](float /*s*/, float d, float sa, float /*da*/) {
    return div255(d * sa);
  });
}

void destination_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](float /*s*/, float d, float sa, float /*da*/) {
    return div255(d * (255.0f - sa));
  });
}

void source_atop(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float sa, float da) {
    return div255(s * da + d * (255.0f - sa));
  });
}

void source_in(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float /*d*/, float /*sa*/, float da) {
    return div255(s * da);
  });
}

void source_out(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float /*d*/, float /*sa*/, float da) {
    return div255(s * (255.0f - da));
  });
}

void source_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float sa, float /*da*/) {
    return s + div255(d * (255.0f - sa));
  });
}

void destination_over(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float /*sa*/, float da) {
    return d + div255(s * (255.0f - da));
  });
}

void modulate(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float /*sa*/, float /*da*/) {
    return div255(s * d);
  });
}

void multiply(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float sa, float da) {
    return div255(s * (255.0f - da) + d * (255.0f - sa) + s * d);
  });
}

void plus(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float /*sa*/, float /*da*/) {
    return std::min(255.0f, s + d);
  });
}

void screen(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float /*sa*/, float /*da*/) {
    return s + d - div255(s * d);
  });
}

void x_or(Pipeline& pipeline) {
  blend_fn(pipeline, [](float s, float d, float sa, float da) {
    return div255(s * (255.0f - da) + d * (255.0f - sa));
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
// Callers compute the pixel offset and pass a span starting at the first pixel.
void load_8888_lowp(std::span<const PremultipliedColorU8> pixels,
                    std::size_t count,
                    LowpChannel& or_, LowpChannel& og,
                    LowpChannel& ob, LowpChannel& oa) {
  for (std::size_t i = 0; i < count; ++i) {
    or_[i] = static_cast<float>(pixels[i].red());
    og[i] = static_cast<float>(pixels[i].green());
    ob[i] = static_cast<float>(pixels[i].blue());
    oa[i] = static_cast<float>(pixels[i].alpha());
  }
  for (std::size_t i = count; i < kStageWidth; ++i) {
    or_[i] = 0.0f;
    og[i] = 0.0f;
    ob[i] = 0.0f;
    oa[i] = 0.0f;
  }
}

// Matches Rust store_8888(&u16x16, ..., &mut [PremultipliedColorU8; STAGE_WIDTH]).
// Callers compute the pixel offset and pass a span starting at the first pixel.
void store_8888_lowp(std::span<PremultipliedColorU8> pixels,
                     std::size_t count,
                     const LowpChannel& r,
                     const LowpChannel& g,
                     const LowpChannel& b,
                     const LowpChannel& a) {
  auto clamp = [](float v) -> std::uint8_t {
    return static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, v)));
  };
  for (std::size_t i = 0; i < count; ++i) {
    pixels[i] = PremultipliedColorU8::fromRgbaUnchecked(
        clamp(r[i]), clamp(g[i]), clamp(b[i]), clamp(a[i]));
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
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    pipeline.da[i] = static_cast<float>(pipeline.pixmap_dst->data[offset + i]);
  }
  for (std::size_t i = pipeline.tail; i < kStageWidth; ++i) {
    pipeline.da[i] = 0.0f;
  }
  pipeline.nextStage();
}

void store_u8(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.dy * pipeline.pixmap_dst->real_width + pipeline.dx;
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    pipeline.pixmap_dst->data[offset + i] = static_cast<std::uint8_t>(pipeline.a[i]);
  }
  pipeline.nextStage();
}

void load_mask_u8(Pipeline& pipeline) {
  if (pipeline.mask_ctx == nullptr || pipeline.mask_ctx->data == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.mask_ctx->byteOffset(pipeline.dx, pipeline.dy);
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    pipeline.a[i] = static_cast<float>(pipeline.mask_ctx->data[offset + i]);
  }
  for (std::size_t i = pipeline.tail; i < kStageWidth; ++i) {
    pipeline.a[i] = 0.0f;
  }
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = 0.0f;
    pipeline.g[i] = 0.0f;
    pipeline.b[i] = 0.0f;
  }
  pipeline.nextStage();
}

void mask_u8(Pipeline& pipeline) {
  if (pipeline.mask_ctx == nullptr || pipeline.mask_ctx->data == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.mask_ctx->byteOffset(pipeline.dx, pipeline.dy);
  LowpChannel c{};
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    c[i] = static_cast<float>(pipeline.mask_ctx->data[offset + i]);
  }
  bool all_zero = true;
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    if (c[i] != 0.0f) {
      all_zero = false;
      break;
    }
  }
  if (all_zero) {
    return;
  }
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = div255(pipeline.r[i] * c[i]);
    pipeline.g[i] = div255(pipeline.g[i] * c[i]);
    pipeline.b[i] = div255(pipeline.b[i] * c[i]);
    pipeline.a[i] = div255(pipeline.a[i] * c[i]);
  }
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
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float inv_sa = 255.0f - pipeline.a[i];
    pipeline.r[i] = pipeline.r[i] + div255(pipeline.dr[i] * inv_sa);
    pipeline.g[i] = pipeline.g[i] + div255(pipeline.dg[i] * inv_sa);
    pipeline.b[i] = pipeline.b[i] + div255(pipeline.db[i] * inv_sa);
    pipeline.a[i] = pipeline.a[i] + div255(pipeline.da[i] * inv_sa);
  }
  store_8888_lowp(pixels, pipeline.tail,
                  pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

void darken(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float sa, float da) {
    return s + d - div255(std::max(s * da, d * sa));
  });
}

void lighten(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float sa, float da) {
    return s + d - div255(std::min(s * da, d * sa));
  });
}

void difference(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float sa, float da) {
    return s + d - 2.0f * div255(std::min(s * da, d * sa));
  });
}

void exclusion(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float /*sa*/, float /*da*/) {
    return s + d - 2.0f * div255(s * d);
  });
}

void hard_light(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float sa, float da) {
    const float inv_da = 255.0f - da, inv_sa = 255.0f - sa;
    const float body = (2.0f * s <= sa)
        ? (2.0f * s * d)
        : (sa * da - 2.0f * (sa - s) * (da - d));
    return div255(s * inv_da + d * inv_sa + body);
  });
}

void overlay(Pipeline& pipeline) {
  blend_fn2(pipeline, [](float s, float d, float sa, float da) {
    const float inv_da = 255.0f - da, inv_sa = 255.0f - sa;
    const float body = (2.0f * d <= da)
        ? (2.0f * s * d)
        : (sa * da - 2.0f * (sa - s) * (da - d));
    return div255(s * inv_da + d * inv_sa + body);
  });
}

// --- Lowp coordinate/shader stages ---
//
// Structural note (WI-07): Rust lowp uses split()/join() to convert between
// f32x16 (a single 512-bit float SIMD vector used for coordinates) and pairs
// of u16x16 (256-bit integer SIMD vectors used for pixel channels). The
// pipeline stores coordinates by splitting one f32x16 across two u16x16
// registers (e.g. x -> r,g and y -> b,a), then join() reconstructs the
// f32x16 before coordinate math.
//
// In C++ lowp, coordinates are already stored as separate float arrays
// (pipeline.r for x, pipeline.g for y, etc.), so no split/join conversion
// is needed — the float arrays serve directly as both coordinate and channel
// storage. The coordinate stages below operate on pipeline.r/g/b/a directly,
// which is equivalent to the Rust pattern of join -> compute -> split.

void transform(Pipeline& pipeline) {
  const auto& ts = pipeline.ctx->transform;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    // Match Rust's nested mad: x * sx + (y * kx + tx)
    pipeline.r[i] = x * ts.sx + (y * ts.kx + ts.tx);
    pipeline.g[i] = x * ts.ky + (y * ts.sy + ts.ty);
  }
  pipeline.nextStage();
}

void pad_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = std::max(0.0f, std::min(1.0f, pipeline.r[i]));
  }
  pipeline.nextStage();
}

void reflect_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float v = pipeline.r[i];
    pipeline.r[i] = std::max(0.0f, std::min(1.0f,
        std::abs((v - 1.0f) - 2.0f * std::floor((v - 1.0f) * 0.5f) - 1.0f)));
  }
  pipeline.nextStage();
}

void repeat_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float v = pipeline.r[i];
    pipeline.r[i] = std::max(0.0f, std::min(1.0f, v - std::floor(v)));
  }
  pipeline.nextStage();
}

void evenly_spaced_2_stop_gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->evenly_spaced_2_stop_gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    pipeline.r[i] = fromFloat(normalize(t * ctx.factor.r + ctx.bias.r));
    pipeline.g[i] = fromFloat(normalize(t * ctx.factor.g + ctx.bias.g));
    pipeline.b[i] = fromFloat(normalize(t * ctx.factor.b + ctx.bias.b));
    pipeline.a[i] = fromFloat(t * ctx.factor.a + ctx.bias.a);
  }
  pipeline.nextStage();
}

void gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    std::uint32_t idx = 0;
    for (std::size_t s = 1; s < ctx.len; ++s) {
      if (t >= ctx.t_values[s]) {
        idx += 1;
      }
    }
    const auto& factor = ctx.factors[idx];
    const auto& bias = ctx.biases[idx];
    pipeline.r[i] = fromFloat(normalize(t * factor.r + bias.r));
    pipeline.g[i] = fromFloat(normalize(t * factor.g + bias.g));
    pipeline.b[i] = fromFloat(normalize(t * factor.b + bias.b));
    pipeline.a[i] = fromFloat(t * factor.a + bias.a);
  }
  pipeline.nextStage();
}

void xy_to_radius(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = std::sqrt(x * x + y * y);
  }
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
