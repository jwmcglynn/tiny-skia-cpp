// Disable FMA contraction to match Rust's highp pipeline, which uses software
// SIMD wrappers (f32x8) that prevent LLVM from fusing multiply-add.
#pragma clang fp contract(off)

#include "tiny_skia/pipeline/Highp.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Math.h"
#include "tiny_skia/Pixmap.h"

namespace tiny_skia::pipeline::highp {

struct Pipeline {
  std::array<float, kStageWidth> r{};
  std::array<float, kStageWidth> g{};
  std::array<float, kStageWidth> b{};
  std::array<float, kStageWidth> a{};
  std::array<float, kStageWidth> dr{};
  std::array<float, kStageWidth> dg{};
  std::array<float, kStageWidth> db{};
  std::array<float, kStageWidth> da{};

  const std::array<StageFn, tiny_skia::pipeline::kMaxStages>* functions = nullptr;
  std::size_t index = 0;
  std::size_t dx = 0;
  std::size_t dy = 0;
  std::size_t tail = 0;
  const ScreenIntRect* rect = nullptr;
  const AAMaskCtx* aa_mask_ctx = nullptr;
  const MaskCtx* mask_ctx = nullptr;
  Context* ctx = nullptr;
  const PixmapRef* pixmap_src = nullptr;
  SubPixmapMut* pixmap_dst = nullptr;

  Pipeline(const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& fun,
           const std::array<StageFn, tiny_skia::pipeline::kMaxStages>&,
           const ScreenIntRect& rect_arg, const AAMaskCtx& aa_mask_ctx_arg,
           const MaskCtx& mask_ctx_arg, Context& ctx_arg, const PixmapRef& pixmap_src_arg,
           SubPixmapMut* pixmap_dst_arg)
      : functions(&fun),
        rect(&rect_arg),
        aa_mask_ctx(&aa_mask_ctx_arg),
        mask_ctx(&mask_ctx_arg),
        ctx(&ctx_arg),
        pixmap_src(&pixmap_src_arg),
        pixmap_dst(pixmap_dst_arg) {}

  void nextStage() {
    const auto next = (*functions)[index];
    ++index;
    next(*this);
  }
};

bool fnPtrEq(StageFn a, StageFn b) { return a == b; }

const void* fnPtr(StageFn fn) { return reinterpret_cast<const void*>(fn); }

namespace {

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

void clamp_0(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = std::max(0.0f, pipeline.r[i]);
    pipeline.g[i] = std::max(0.0f, pipeline.g[i]);
    pipeline.b[i] = std::max(0.0f, pipeline.b[i]);
    pipeline.a[i] = std::max(0.0f, pipeline.a[i]);
  }
  pipeline.nextStage();
}

void clamp_a(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = std::min(1.0f, pipeline.r[i]);
    pipeline.g[i] = std::min(1.0f, pipeline.g[i]);
    pipeline.b[i] = std::min(1.0f, pipeline.b[i]);
    pipeline.a[i] = std::min(1.0f, pipeline.a[i]);
  }
  pipeline.nextStage();
}

void premultiply(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= pipeline.a[i];
    pipeline.g[i] *= pipeline.a[i];
    pipeline.b[i] *= pipeline.a[i];
  }
  pipeline.nextStage();
}

void uniform_color(Pipeline& pipeline) {
  const auto& u = pipeline.ctx->uniform_color;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = u.r;
    pipeline.g[i] = u.g;
    pipeline.b[i] = u.b;
    pipeline.a[i] = u.a;
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
  const std::array<float, kStageWidth> c{
      static_cast<float>(data[0]) / 255.0f,
      static_cast<float>(data[1]) / 255.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
  };

  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= c[i];
    pipeline.g[i] *= c[i];
    pipeline.b[i] *= c[i];
    pipeline.a[i] *= c[i];
  }
  pipeline.nextStage();
}

void lerp_u8(Pipeline& pipeline) {
  const auto data = pipeline.aa_mask_ctx->copyAtXY(pipeline.dx, pipeline.dy, pipeline.tail);
  const std::array<float, kStageWidth> c{
      static_cast<float>(data[0]) / 255.0f,
      static_cast<float>(data[1]) / 255.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
  };

  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.dr[i] + (pipeline.r[i] - pipeline.dr[i]) * c[i];
    pipeline.g[i] = pipeline.dg[i] + (pipeline.g[i] - pipeline.dg[i]) * c[i];
    pipeline.b[i] = pipeline.db[i] + (pipeline.b[i] - pipeline.db[i]) * c[i];
    pipeline.a[i] = pipeline.da[i] + (pipeline.a[i] - pipeline.da[i]) * c[i];
  }
  pipeline.nextStage();
}

void scale_1_float(Pipeline& pipeline) {
  const auto c = pipeline.ctx->current_coverage;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= c;
    pipeline.g[i] *= c;
    pipeline.b[i] *= c;
    pipeline.a[i] *= c;
  }
  pipeline.nextStage();
}

void lerp_1_float(Pipeline& pipeline) {
  const auto c = pipeline.ctx->current_coverage;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.dr[i] + (pipeline.r[i] - pipeline.dr[i]) * c;
    pipeline.g[i] = pipeline.dg[i] + (pipeline.g[i] - pipeline.dg[i]) * c;
    pipeline.b[i] = pipeline.db[i] + (pipeline.b[i] - pipeline.db[i]) * c;
    pipeline.a[i] = pipeline.da[i] + (pipeline.a[i] - pipeline.da[i]) * c;
  }
  pipeline.nextStage();
}

void clear(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = 0.0f;
    pipeline.g[i] = 0.0f;
    pipeline.b[i] = 0.0f;
    pipeline.a[i] = 0.0f;
  }
  pipeline.nextStage();
}

void source_atop(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto s = pipeline.r[i];
    const auto d = pipeline.dr[i];
    const auto sa = pipeline.a[i];
    const auto da = pipeline.da[i];
    pipeline.r[i] = s * da + d * (1.0f - sa);
    pipeline.g[i] = pipeline.g[i] * da + pipeline.dg[i] * (1.0f - sa);
    pipeline.b[i] = pipeline.b[i] * da + pipeline.db[i] * (1.0f - sa);
    pipeline.a[i] = pipeline.a[i] * da + pipeline.da[i] * (1.0f - pipeline.a[i]);
  }
  pipeline.nextStage();
}

void source_in(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= pipeline.da[i];
    pipeline.g[i] *= pipeline.da[i];
    pipeline.b[i] *= pipeline.da[i];
    pipeline.a[i] *= pipeline.da[i];
  }
  pipeline.nextStage();
}

void source_out(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto daInv = 1.0f - pipeline.da[i];
    pipeline.r[i] *= daInv;
    pipeline.g[i] *= daInv;
    pipeline.b[i] *= daInv;
    pipeline.a[i] *= daInv;
  }
  pipeline.nextStage();
}

void destination_atop(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto s = pipeline.r[i];
    const auto d = pipeline.dr[i];
    const auto sa = pipeline.a[i];
    const auto da = pipeline.da[i];
    pipeline.r[i] = d * sa + s * (1.0f - da);
    pipeline.g[i] = pipeline.g[i] * sa + pipeline.dg[i] * (1.0f - da);
    pipeline.b[i] = pipeline.b[i] * sa + pipeline.db[i] * (1.0f - da);
    pipeline.a[i] = pipeline.da[i] * pipeline.a[i] + pipeline.a[i] * (1.0f - pipeline.da[i]);
  }
  pipeline.nextStage();
}

void destination_in(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto sa = pipeline.a[i];
    pipeline.r[i] = pipeline.dr[i] * sa;
    pipeline.g[i] = pipeline.dg[i] * sa;
    pipeline.b[i] = pipeline.db[i] * sa;
    pipeline.a[i] = pipeline.da[i] * sa;
  }
  pipeline.nextStage();
}

void destination_out(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto saInv = 1.0f - pipeline.a[i];
    pipeline.r[i] = pipeline.dr[i] * saInv;
    pipeline.g[i] = pipeline.dg[i] * saInv;
    pipeline.b[i] = pipeline.db[i] * saInv;
    pipeline.a[i] = pipeline.da[i] * saInv;
  }
  pipeline.nextStage();
}

void source_over(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto saInv = 1.0f - pipeline.a[i];
    pipeline.r[i] = pipeline.dr[i] * saInv + pipeline.r[i];
    pipeline.g[i] = pipeline.dg[i] * saInv + pipeline.g[i];
    pipeline.b[i] = pipeline.db[i] * saInv + pipeline.b[i];
    pipeline.a[i] = pipeline.da[i] * saInv + pipeline.a[i];
  }
  pipeline.nextStage();
}

void destination_over(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto daInv = 1.0f - pipeline.da[i];
    pipeline.r[i] = pipeline.r[i] * daInv + pipeline.dr[i];
    pipeline.g[i] = pipeline.g[i] * daInv + pipeline.dg[i];
    pipeline.b[i] = pipeline.b[i] * daInv + pipeline.db[i];
    pipeline.a[i] = pipeline.da[i] + pipeline.a[i] * daInv;
  }
  pipeline.nextStage();
}

void modulate(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= pipeline.dr[i];
    pipeline.g[i] *= pipeline.dg[i];
    pipeline.b[i] *= pipeline.db[i];
    pipeline.a[i] *= pipeline.da[i];
  }
  pipeline.nextStage();
}

void multiply(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto inv_da = 1.0f - pipeline.da[i];
    const auto inv_sa = 1.0f - pipeline.a[i];
    pipeline.r[i] =
        pipeline.r[i] * inv_da + pipeline.dr[i] * inv_sa + pipeline.r[i] * pipeline.dr[i];
    pipeline.g[i] =
        pipeline.g[i] * inv_da + pipeline.dg[i] * inv_sa + pipeline.g[i] * pipeline.dg[i];
    pipeline.b[i] =
        pipeline.b[i] * inv_da + pipeline.db[i] * inv_sa + pipeline.b[i] * pipeline.db[i];
    pipeline.a[i] =
        pipeline.a[i] * inv_da + pipeline.da[i] * inv_sa + pipeline.a[i] * pipeline.da[i];
  }
  pipeline.nextStage();
}

void plus(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = std::min(1.0f, pipeline.r[i] + pipeline.dr[i]);
    pipeline.g[i] = std::min(1.0f, pipeline.g[i] + pipeline.dg[i]);
    pipeline.b[i] = std::min(1.0f, pipeline.b[i] + pipeline.db[i]);
    pipeline.a[i] = std::min(1.0f, pipeline.a[i] + pipeline.da[i]);
  }
  pipeline.nextStage();
}

void screen(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.r[i] + pipeline.dr[i] - pipeline.r[i] * pipeline.dr[i];
    pipeline.g[i] = pipeline.g[i] + pipeline.dg[i] - pipeline.g[i] * pipeline.dg[i];
    pipeline.b[i] = pipeline.b[i] + pipeline.db[i] - pipeline.b[i] * pipeline.db[i];
    pipeline.a[i] = pipeline.a[i] + pipeline.da[i] - pipeline.a[i] * pipeline.da[i];
  }
  pipeline.nextStage();
}

void x_or(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto inv_da = 1.0f - pipeline.da[i];
    const auto inv_sa = 1.0f - pipeline.a[i];
    pipeline.r[i] = pipeline.r[i] * inv_da + pipeline.dr[i] * inv_sa;
    pipeline.g[i] = pipeline.g[i] * inv_da + pipeline.dg[i] * inv_sa;
    pipeline.b[i] = pipeline.b[i] * inv_da + pipeline.db[i] * inv_sa;
    pipeline.a[i] = pipeline.a[i] * inv_da + pipeline.da[i] * inv_sa;
  }
  pipeline.nextStage();
}

void color_burn(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto s = pipeline.r[i];
    const auto d = pipeline.dr[i];
    const auto sa = pipeline.a[i];
    const auto da = pipeline.da[i];
    const auto inv_da = 1.0f - da;
    const auto inv_sa = 1.0f - sa;
    if (d == da) {
      pipeline.r[i] = d + s * inv_da;
    } else if (s == 0.0f) {
      pipeline.r[i] = d * inv_sa;
    } else {
      pipeline.r[i] = sa * (da - std::min(da, (da - d) * sa / s)) + s * inv_da + d * inv_sa;
    }
    const auto sg = pipeline.g[i];
    const auto dg = pipeline.dg[i];
    if (dg == da) {
      pipeline.g[i] = dg + sg * inv_da;
    } else if (sg == 0.0f) {
      pipeline.g[i] = dg * inv_sa;
    } else {
      pipeline.g[i] = sa * (da - std::min(da, (da - dg) * sa / sg)) + sg * inv_da + dg * inv_sa;
    }
    const auto sb = pipeline.b[i];
    const auto db = pipeline.db[i];
    if (db == da) {
      pipeline.b[i] = db + sb * inv_da;
    } else if (sb == 0.0f) {
      pipeline.b[i] = db * inv_sa;
    } else {
      pipeline.b[i] = sa * (da - std::min(da, (da - db) * sa / sb)) + sb * inv_da + db * inv_sa;
    }
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

void color_dodge(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto s = pipeline.r[i];
    const auto d = pipeline.dr[i];
    const auto sa = pipeline.a[i];
    const auto da = pipeline.da[i];
    const auto inv_da = 1.0f - da;
    const auto inv_sa = 1.0f - sa;
    if (d == 0.0f) {
      pipeline.r[i] = s * inv_da;
    } else if (s == sa) {
      pipeline.r[i] = s + d * inv_sa;
    } else {
      pipeline.r[i] = sa * std::min(da, (d * sa) / (sa - s)) + s * inv_da + d * inv_sa;
    }
    const auto sg = pipeline.g[i];
    const auto dg = pipeline.dg[i];
    if (dg == 0.0f) {
      pipeline.g[i] = sg * inv_da;
    } else if (sg == sa) {
      pipeline.g[i] = sg + dg * inv_sa;
    } else {
      pipeline.g[i] = sa * std::min(da, (dg * sa) / (sa - sg)) + sg * inv_da + dg * inv_sa;
    }
    const auto sb = pipeline.b[i];
    const auto db = pipeline.db[i];
    if (db == 0.0f) {
      pipeline.b[i] = sb * inv_da;
    } else if (sb == sa) {
      pipeline.b[i] = sb + db * inv_sa;
    } else {
      pipeline.b[i] = sa * std::min(da, (db * sa) / (sa - sb)) + sb * inv_da + db * inv_sa;
    }
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

constexpr float kInv255 = 1.0f / 255.0f;

void load_8888(const std::uint8_t* data, std::size_t stride, std::size_t dx, std::size_t dy,
               std::size_t count, Pipeline& p, std::array<float, kStageWidth>& or_,
               std::array<float, kStageWidth>& og, std::array<float, kStageWidth>& ob,
               std::array<float, kStageWidth>& oa) {
  const auto offset = (dy * stride + dx) * 4;
  for (std::size_t i = 0; i < count; ++i) {
    const auto base = offset + i * 4;
    or_[i] = static_cast<float>(data[base + 0]) * kInv255;
    og[i] = static_cast<float>(data[base + 1]) * kInv255;
    ob[i] = static_cast<float>(data[base + 2]) * kInv255;
    oa[i] = static_cast<float>(data[base + 3]) * kInv255;
  }
  for (std::size_t i = count; i < kStageWidth; ++i) {
    or_[i] = 0.0f;
    og[i] = 0.0f;
    ob[i] = 0.0f;
    oa[i] = 0.0f;
  }
  (void)p;
}

// Clamp to [0,1], multiply by 255, round-to-nearest-even.
// Uses std::nearbyintf which follows the current rounding mode (FE_TONEAREST by default),
// matching ARM NEON vcvtnq_s32_f32 used by Rust's round_int().
inline std::uint8_t unnorm(float v) {
  float clamped = std::max(0.0f, std::min(1.0f, v));
  return static_cast<std::uint8_t>(std::nearbyintf(clamped * 255.0f));
}

void store_8888(std::uint8_t* data, std::size_t stride, std::size_t dx, std::size_t dy,
                std::size_t count, const std::array<float, kStageWidth>& r,
                const std::array<float, kStageWidth>& g, const std::array<float, kStageWidth>& b,
                const std::array<float, kStageWidth>& a) {
  const auto offset = (dy * stride + dx) * 4;
  for (std::size_t i = 0; i < count; ++i) {
    const auto base = offset + i * 4;
    data[base + 0] = unnorm(r[i]);
    data[base + 1] = unnorm(g[i]);
    data[base + 2] = unnorm(b[i]);
    data[base + 3] = unnorm(a[i]);
  }
}

void load_dst(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  load_8888(pipeline.pixmap_dst->data, pipeline.pixmap_dst->realWidth, pipeline.dx, pipeline.dy,
            pipeline.tail, pipeline, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  pipeline.nextStage();
}

void store(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  store_8888(pipeline.pixmap_dst->data, pipeline.pixmap_dst->realWidth, pipeline.dx, pipeline.dy,
             pipeline.tail, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

// U8 (mask) stages are unreachable in highp; all mask/A8 pixmaps use lowp.
void load_dst_u8(Pipeline& pipeline) { pipeline.nextStage(); }
void store_u8(Pipeline& pipeline) { pipeline.nextStage(); }
void load_mask_u8(Pipeline& pipeline) { pipeline.nextStage(); }

void mask_u8(Pipeline& pipeline) {
  if (pipeline.mask_ctx == nullptr || pipeline.mask_ctx->data == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto offset = pipeline.mask_ctx->byteOffset(pipeline.dx, pipeline.dy);
  std::array<float, kStageWidth> c{};
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    c[i] = static_cast<float>(pipeline.mask_ctx->data[offset + i]) * kInv255;
  }
  bool all_zero = true;
  for (std::size_t i = 0; i < pipeline.tail; ++i) {
    if (c[i] != 0.0f) {
      all_zero = false;
      break;
    }
  }
  if (all_zero) {
    return;  // Early return, skip remaining stages.
  }
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= c[i];
    pipeline.g[i] *= c[i];
    pipeline.b[i] *= c[i];
    pipeline.a[i] *= c[i];
  }
  pipeline.nextStage();
}

void source_over_rgba(Pipeline& pipeline) {
  if (pipeline.pixmap_dst == nullptr) {
    pipeline.nextStage();
    return;
  }
  load_8888(pipeline.pixmap_dst->data, pipeline.pixmap_dst->realWidth, pipeline.dx, pipeline.dy,
            pipeline.tail, pipeline, pipeline.dr, pipeline.dg, pipeline.db, pipeline.da);
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto inv_a = 1.0f - pipeline.a[i];
    pipeline.r[i] = pipeline.dr[i] * inv_a + pipeline.r[i];
    pipeline.g[i] = pipeline.dg[i] * inv_a + pipeline.g[i];
    pipeline.b[i] = pipeline.db[i] * inv_a + pipeline.b[i];
    pipeline.a[i] = pipeline.da[i] * inv_a + pipeline.a[i];
  }
  store_8888(pipeline.pixmap_dst->data, pipeline.pixmap_dst->realWidth, pipeline.dx, pipeline.dy,
             pipeline.tail, pipeline.r, pipeline.g, pipeline.b, pipeline.a);
  pipeline.nextStage();
}

// ---------------------------------------------------------------------------
// Helper functions for pipeline stages
// ---------------------------------------------------------------------------

/// Subtract one ULP (unit in the last place) from a float.
/// Converts exclusive upper bound to inclusive: e.g. width 100 -> 99.999...
inline float ulp_sub(float v) {
  auto bits = std::bit_cast<std::uint32_t>(v);
  bits -= 1;
  return std::bit_cast<float>(bits);
}

/// Clamp a float to [0, 1].
inline float normalize(float v) { return std::max(0.0f, std::min(1.0f, v)); }

/// Fused multiply-add: f * m + a.
inline float mad(float f, float m, float a) { return f * m + a; }

/// Repeat tiling for coordinates in [0, limit) range.
inline float exclusive_repeat(float v, float limit, float inv_limit) {
  return v - std::floor(v * inv_limit) * limit;
}

/// Reflect tiling for coordinates in [0, limit) range.
inline float exclusive_reflect(float v, float limit, float inv_limit) {
  return std::abs((v - limit) - (limit + limit) * std::floor((v - limit) * (inv_limit * 0.5f)) -
                  limit);
}

/// Apply tile mode to a coordinate.
inline float tile(float v, SpreadMode mode, float limit, float inv_limit) {
  switch (mode) {
    case SpreadMode::Pad:
      return v;
    case SpreadMode::Repeat:
      return exclusive_repeat(v, limit, inv_limit);
    case SpreadMode::Reflect:
      return exclusive_reflect(v, limit, inv_limit);
  }
  return v;
}

/// Compute pixel indices from clamped x,y coordinates.
inline std::uint32_t gather_ix_scalar(const PixmapRef& pixmap, float x, float y) {
  const float w = ulp_sub(static_cast<float>(pixmap.width()));
  const float h = ulp_sub(static_cast<float>(pixmap.height()));
  x = std::max(0.0f, std::min(w, x));
  y = std::max(0.0f, std::min(h, y));
  return static_cast<std::uint32_t>(static_cast<std::int32_t>(y)) * pixmap.width() +
         static_cast<std::uint32_t>(static_cast<std::int32_t>(x));
}

/// Load a gathered pixel into float channels.
inline void load_gathered_pixel(const PixmapRef& pixmap, std::uint32_t ix, float& r, float& g,
                                float& b, float& a) {
  const auto pixels = pixmap.pixels();
  const auto idx = std::min(static_cast<std::size_t>(ix), pixels.size() - 1);
  const auto& px = pixels[idx];
  r = static_cast<float>(px.red()) * kInv255;
  g = static_cast<float>(px.green()) * kInv255;
  b = static_cast<float>(px.blue()) * kInv255;
  a = static_cast<float>(px.alpha()) * kInv255;
}

/// Sample a single pixel with tiling applied.
inline void sample(const PixmapRef& pixmap, const SamplerCtx& ctx, float x, float y, float& r,
                   float& g, float& b, float& a) {
  x = tile(x, ctx.spread_mode, static_cast<float>(pixmap.width()), ctx.inv_width);
  y = tile(y, ctx.spread_mode, static_cast<float>(pixmap.height()), ctx.inv_height);
  const auto ix = gather_ix_scalar(pixmap, x, y);
  load_gathered_pixel(pixmap, ix, r, g, b, a);
}

/// Bicubic near weight: 1/18 + 9/18*t + 27/18*t^2 - 21/18*t^3
inline float bicubic_near(float t) {
  return mad(t, mad(t, mad(-21.0f / 18.0f, t, 27.0f / 18.0f), 9.0f / 18.0f), 1.0f / 18.0f);
}

/// Bicubic far weight: t^2 * (7/18*t - 6/18)
inline float bicubic_far(float t) { return (t * t) * mad(7.0f / 18.0f, t, -6.0f / 18.0f); }

/// HSL saturation: max(r,g,b) - min(r,g,b)
inline float sat(float r, float g, float b) { return std::max({r, g, b}) - std::min({r, g, b}); }

/// HSL luminosity: 0.30*r + 0.59*g + 0.11*b
inline float lum(float r, float g, float b) { return r * 0.30f + g * 0.59f + b * 0.11f; }

/// Set saturation of (r,g,b) to target saturation s.
inline void set_sat(float& r, float& g, float& b, float s) {
  const float mn = std::min({r, g, b});
  const float mx = std::max({r, g, b});
  const float satv = mx - mn;
  auto scale = [&](float c) -> float { return satv == 0.0f ? 0.0f : (c - mn) * s / satv; };
  r = scale(r);
  g = scale(g);
  b = scale(b);
}

/// Set luminosity of (r,g,b) to target luminosity l.
inline void set_lum(float& r, float& g, float& b, float l) {
  const float diff = l - lum(r, g, b);
  r += diff;
  g += diff;
  b += diff;
}

/// Clip color to valid premultiplied range.
inline void clip_color(float& r, float& g, float& b, float a) {
  const float mn = std::min({r, g, b});
  const float mx = std::max({r, g, b});
  const float l = lum(r, g, b);
  auto clip = [&](float c) -> float {
    if (mn < 0.0f) {
      c = l + (c - l) * l / (l - mn);
    }
    if (mx > a) {
      c = l + (c - l) * (a - l) / (mx - l);
    }
    return std::max(0.0f, c);
  };
  r = clip(r);
  g = clip(g);
  b = clip(b);
}

/// sRGB expand: linear -> sRGB
inline float srgb_expand_scalar(float x) {
  if (x <= 0.04045f) {
    return x / 12.92f;
  }
  return approxPowf((x + 0.055f) / 1.055f, 2.4f);
}

/// sRGB compress: sRGB -> linear
inline float srgb_compress_scalar(float x) {
  if (x <= 0.0031308f) {
    return x * 12.92f;
  }
  return approxPowf(x, 0.416666666f) * 1.055f - 0.055f;
}

/// Bitwise AND a float with a uint32 mask (reinterpret cast).
inline float float_and_mask(float v, std::uint32_t mask) {
  auto bits = std::bit_cast<std::uint32_t>(v);
  bits &= mask;
  return std::bit_cast<float>(bits);
}

// ---------------------------------------------------------------------------
// Pipeline stage implementations
// ---------------------------------------------------------------------------

void gather(Pipeline& pipeline) {
  if (pipeline.pixmap_src == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto& pixmap = *pipeline.pixmap_src;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const auto ix = gather_ix_scalar(pixmap, pipeline.r[i], pipeline.g[i]);
    load_gathered_pixel(pixmap, ix, pipeline.r[i], pipeline.g[i], pipeline.b[i], pipeline.a[i]);
  }
  pipeline.nextStage();
}

void transform(Pipeline& pipeline) {
  const auto& ts = pipeline.ctx->transform;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = mad(x, ts.sx, mad(y, ts.kx, ts.tx));
    pipeline.g[i] = mad(x, ts.ky, mad(y, ts.sy, ts.ty));
  }
  pipeline.nextStage();
}

void reflect(Pipeline& pipeline) {
  const auto& lx = pipeline.ctx->limit_x;
  const auto& ly = pipeline.ctx->limit_y;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = exclusive_reflect(pipeline.r[i], lx.scale, lx.inv_scale);
    pipeline.g[i] = exclusive_reflect(pipeline.g[i], ly.scale, ly.inv_scale);
  }
  pipeline.nextStage();
}

void repeat(Pipeline& pipeline) {
  const auto& lx = pipeline.ctx->limit_x;
  const auto& ly = pipeline.ctx->limit_y;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = exclusive_repeat(pipeline.r[i], lx.scale, lx.inv_scale);
    pipeline.g[i] = exclusive_repeat(pipeline.g[i], ly.scale, ly.inv_scale);
  }
  pipeline.nextStage();
}

void bilinear(Pipeline& pipeline) {
  if (pipeline.pixmap_src == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto& pixmap = *pipeline.pixmap_src;
  const auto& ctx = pipeline.ctx->sampler;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float cx = pipeline.r[i];
    const float cy = pipeline.g[i];
    const float fx = (cx + 0.5f) - std::floor(cx + 0.5f);
    const float fy = (cy + 0.5f) - std::floor(cy + 0.5f);
    const float wx[2] = {1.0f - fx, fx};
    const float wy[2] = {1.0f - fy, fy};

    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    float y = cy - 0.5f;
    for (int j = 0; j < 2; ++j) {
      float x = cx - 0.5f;
      for (int k = 0; k < 2; ++k) {
        float sr, sg, sb, sa;
        sample(pixmap, ctx, x, y, sr, sg, sb, sa);
        const float w = wx[k] * wy[j];
        r = mad(w, sr, r);
        g = mad(w, sg, g);
        b = mad(w, sb, b);
        a = mad(w, sa, a);
        x += 1.0f;
      }
      y += 1.0f;
    }
    pipeline.r[i] = r;
    pipeline.g[i] = g;
    pipeline.b[i] = b;
    pipeline.a[i] = a;
  }
  pipeline.nextStage();
}

void bicubic(Pipeline& pipeline) {
  if (pipeline.pixmap_src == nullptr) {
    pipeline.nextStage();
    return;
  }
  const auto& pixmap = *pipeline.pixmap_src;
  const auto& ctx = pipeline.ctx->sampler;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float cx = pipeline.r[i];
    const float cy = pipeline.g[i];
    const float fx = (cx + 0.5f) - std::floor(cx + 0.5f);
    const float fy = (cy + 0.5f) - std::floor(cy + 0.5f);
    const float wx[4] = {bicubic_far(1.0f - fx), bicubic_near(1.0f - fx), bicubic_near(fx),
                         bicubic_far(fx)};
    const float wy[4] = {bicubic_far(1.0f - fy), bicubic_near(1.0f - fy), bicubic_near(fy),
                         bicubic_far(fy)};

    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    float y = cy - 1.5f;
    for (int j = 0; j < 4; ++j) {
      float x = cx - 1.5f;
      for (int k = 0; k < 4; ++k) {
        float sr, sg, sb, sa;
        sample(pixmap, ctx, x, y, sr, sg, sb, sa);
        const float w = wx[k] * wy[j];
        r = mad(w, sr, r);
        g = mad(w, sg, g);
        b = mad(w, sb, b);
        a = mad(w, sa, a);
        x += 1.0f;
      }
      y += 1.0f;
    }
    pipeline.r[i] = r;
    pipeline.g[i] = g;
    pipeline.b[i] = b;
    pipeline.a[i] = a;
  }
  pipeline.nextStage();
}

void pad_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = normalize(pipeline.r[i]);
  }
  pipeline.nextStage();
}

void reflect_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float v = pipeline.r[i];
    pipeline.r[i] = normalize(std::abs((v - 1.0f) - 2.0f * std::floor((v - 1.0f) * 0.5f) - 1.0f));
  }
  pipeline.nextStage();
}

void repeat_x1(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float v = pipeline.r[i];
    pipeline.r[i] = normalize(v - std::floor(v));
  }
  pipeline.nextStage();
}

void evenly_spaced_2_stop_gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->evenly_spaced_2_stop_gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    pipeline.r[i] = mad(t, ctx.factor.r, ctx.bias.r);
    pipeline.g[i] = mad(t, ctx.factor.g, ctx.bias.g);
    pipeline.b[i] = mad(t, ctx.factor.b, ctx.bias.b);
    pipeline.a[i] = mad(t, ctx.factor.a, ctx.bias.a);
  }
  pipeline.nextStage();
}

void gradient(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    // Find which stop index to use by counting how many t_values are <= t.
    // Loop starts at 1 because idx 0 is the color before the first stop.
    std::uint32_t idx = 0;
    for (std::size_t s = 1; s < ctx.len; ++s) {
      if (t >= ctx.t_values[s]) {
        idx += 1;
      }
    }
    // Lookup factor and bias for this stop and interpolate.
    const auto& factor = ctx.factors[idx];
    const auto& bias = ctx.biases[idx];
    pipeline.r[i] = mad(t, factor.r, bias.r);
    pipeline.g[i] = mad(t, factor.g, bias.g);
    pipeline.b[i] = mad(t, factor.b, bias.b);
    pipeline.a[i] = mad(t, factor.a, bias.a);
  }
  pipeline.nextStage();
}

void xy_to_unit_angle(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    const float x_abs = std::abs(x);
    const float y_abs = std::abs(y);
    const float max_abs = std::max(x_abs, y_abs);
    const float min_abs = std::min(x_abs, y_abs);
    const float slope = (max_abs == 0.0f) ? 0.0f : min_abs / max_abs;
    const float s = slope * slope;
    // 7th degree polynomial approximation of atan/(2*pi)
    float phi =
        slope *
        (0.15912117063999176025390625f +
         s * (-5.185396969318389892578125e-2f +
              s * (2.476101927459239959716796875e-2f + s * (-7.0547382347285747528076171875e-3f))));
    if (x_abs < y_abs) phi = 0.25f - phi;
    if (x < 0.0f) phi = 0.5f - phi;
    if (y < 0.0f) phi = 1.0f - phi;
    // NaN check: if phi != phi, set to 0.
    if (phi != phi) phi = 0.0f;
    pipeline.r[i] = phi;
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

// --- Separable blend modes ---

void darken(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sa = pipeline.a[i];
    const float da = pipeline.da[i];
    pipeline.r[i] =
        pipeline.r[i] + pipeline.dr[i] - std::max(pipeline.r[i] * da, pipeline.dr[i] * sa);
    pipeline.g[i] =
        pipeline.g[i] + pipeline.dg[i] - std::max(pipeline.g[i] * da, pipeline.dg[i] * sa);
    pipeline.b[i] =
        pipeline.b[i] + pipeline.db[i] - std::max(pipeline.b[i] * da, pipeline.db[i] * sa);
    pipeline.a[i] = mad(da, 1.0f - sa, sa);
  }
  pipeline.nextStage();
}

void lighten(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sa = pipeline.a[i];
    const float da = pipeline.da[i];
    pipeline.r[i] =
        pipeline.r[i] + pipeline.dr[i] - std::min(pipeline.r[i] * da, pipeline.dr[i] * sa);
    pipeline.g[i] =
        pipeline.g[i] + pipeline.dg[i] - std::min(pipeline.g[i] * da, pipeline.dg[i] * sa);
    pipeline.b[i] =
        pipeline.b[i] + pipeline.db[i] - std::min(pipeline.b[i] * da, pipeline.db[i] * sa);
    pipeline.a[i] = mad(da, 1.0f - sa, sa);
  }
  pipeline.nextStage();
}

void difference(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sa = pipeline.a[i];
    const float da = pipeline.da[i];
    pipeline.r[i] =
        pipeline.r[i] + pipeline.dr[i] - 2.0f * std::min(pipeline.r[i] * da, pipeline.dr[i] * sa);
    pipeline.g[i] =
        pipeline.g[i] + pipeline.dg[i] - 2.0f * std::min(pipeline.g[i] * da, pipeline.dg[i] * sa);
    pipeline.b[i] =
        pipeline.b[i] + pipeline.db[i] - 2.0f * std::min(pipeline.b[i] * da, pipeline.db[i] * sa);
    pipeline.a[i] = mad(da, 1.0f - sa, sa);
  }
  pipeline.nextStage();
}

void exclusion(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.r[i] + pipeline.dr[i] - 2.0f * pipeline.r[i] * pipeline.dr[i];
    pipeline.g[i] = pipeline.g[i] + pipeline.dg[i] - 2.0f * pipeline.g[i] * pipeline.dg[i];
    pipeline.b[i] = pipeline.b[i] + pipeline.db[i] - 2.0f * pipeline.b[i] * pipeline.db[i];
    pipeline.a[i] = mad(pipeline.da[i], 1.0f - pipeline.a[i], pipeline.a[i]);
  }
  pipeline.nextStage();
}

void hard_light(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float s = pipeline.r[i], d = pipeline.dr[i];
    const float sa = pipeline.a[i], da = pipeline.da[i];
    pipeline.r[i] = s * (1.0f - da) + d * (1.0f - sa) +
                    ((2.0f * s <= sa) ? 2.0f * s * d : sa * da - 2.0f * (da - d) * (sa - s));
    const float sg = pipeline.g[i], dg = pipeline.dg[i];
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) +
                    ((2.0f * sg <= sa) ? 2.0f * sg * dg : sa * da - 2.0f * (da - dg) * (sa - sg));
    const float sb = pipeline.b[i], db = pipeline.db[i];
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) +
                    ((2.0f * sb <= sa) ? 2.0f * sb * db : sa * da - 2.0f * (da - db) * (sa - sb));
    pipeline.a[i] = mad(da, 1.0f - sa, sa);
  }
  pipeline.nextStage();
}

void overlay(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float s = pipeline.r[i], d = pipeline.dr[i];
    const float sa = pipeline.a[i], da = pipeline.da[i];
    pipeline.r[i] = s * (1.0f - da) + d * (1.0f - sa) +
                    ((2.0f * d <= da) ? 2.0f * s * d : sa * da - 2.0f * (da - d) * (sa - s));
    const float sg = pipeline.g[i], dg = pipeline.dg[i];
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) +
                    ((2.0f * dg <= da) ? 2.0f * sg * dg : sa * da - 2.0f * (da - dg) * (sa - sg));
    const float sb = pipeline.b[i], db = pipeline.db[i];
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) +
                    ((2.0f * db <= da) ? 2.0f * sb * db : sa * da - 2.0f * (da - db) * (sa - sb));
    pipeline.a[i] = mad(da, 1.0f - sa, sa);
  }
  pipeline.nextStage();
}

void soft_light(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float s = pipeline.r[i], d = pipeline.dr[i];
    const float sa = pipeline.a[i], da = pipeline.da[i];
    const float inv_sa = 1.0f - sa;

    auto soft_light_channel = [](float sc, float dc, float sca, float dca) -> float {
      const float m = (dca > 0.0f) ? dc / dca : 0.0f;
      const float s2 = 2.0f * sc;
      const float m4 = 4.0f * m;
      const float dark_src = dc * (sca + (s2 - sca) * (1.0f - m));
      const float dark_dst = (m4 * m4 + m4) * (m - 1.0f) + 7.0f * m;
      const float lite_dst = std::sqrt(m) - m;
      const float lite_src =
          dc * sca + dca * (s2 - sca) * ((4.0f * dc <= dca) ? dark_dst : lite_dst);
      return sc * (1.0f - dca) + dc * (1.0f - sca) + ((s2 <= sca) ? dark_src : lite_src);
    };

    pipeline.r[i] = soft_light_channel(s, d, sa, da);
    pipeline.g[i] = soft_light_channel(pipeline.g[i], pipeline.dg[i], sa, da);
    pipeline.b[i] = soft_light_channel(pipeline.b[i], pipeline.db[i], sa, da);
    pipeline.a[i] = mad(da, inv_sa, sa);
  }
  pipeline.nextStage();
}

// --- Non-separable blend modes ---

void hue(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sr = pipeline.r[i], sg = pipeline.g[i], sb = pipeline.b[i];
    const float sa = pipeline.a[i];
    const float dr = pipeline.dr[i], dg = pipeline.dg[i], db = pipeline.db[i];
    const float da = pipeline.da[i];

    float rr = sr * sa, gg = sg * sa, bb = sb * sa;
    set_sat(rr, gg, bb, sat(dr, dg, db) * sa);
    set_lum(rr, gg, bb, lum(dr, dg, db) * sa);
    clip_color(rr, gg, bb, sa * da);

    pipeline.r[i] = sr * (1.0f - da) + dr * (1.0f - sa) + rr;
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) + gg;
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) + bb;
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

void saturation(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sr = pipeline.r[i], sg = pipeline.g[i], sb = pipeline.b[i];
    const float sa = pipeline.a[i];
    const float dr = pipeline.dr[i], dg = pipeline.dg[i], db = pipeline.db[i];
    const float da = pipeline.da[i];

    float rr = dr * sa, gg = dg * sa, bb = db * sa;
    set_sat(rr, gg, bb, sat(sr, sg, sb) * da);
    set_lum(rr, gg, bb, lum(dr, dg, db) * sa);
    clip_color(rr, gg, bb, sa * da);

    pipeline.r[i] = sr * (1.0f - da) + dr * (1.0f - sa) + rr;
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) + gg;
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) + bb;
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

void color(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sr = pipeline.r[i], sg = pipeline.g[i], sb = pipeline.b[i];
    const float sa = pipeline.a[i];
    const float dr = pipeline.dr[i], dg = pipeline.dg[i], db = pipeline.db[i];
    const float da = pipeline.da[i];

    float rr = sr * da, gg = sg * da, bb = sb * da;
    set_lum(rr, gg, bb, lum(dr, dg, db) * sa);
    clip_color(rr, gg, bb, sa * da);

    pipeline.r[i] = sr * (1.0f - da) + dr * (1.0f - sa) + rr;
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) + gg;
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) + bb;
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

void luminosity(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float sr = pipeline.r[i], sg = pipeline.g[i], sb = pipeline.b[i];
    const float sa = pipeline.a[i];
    const float dr = pipeline.dr[i], dg = pipeline.dg[i], db = pipeline.db[i];
    const float da = pipeline.da[i];

    float rr = dr * sa, gg = dg * sa, bb = db * sa;
    set_lum(rr, gg, bb, lum(sr, sg, sb) * da);
    clip_color(rr, gg, bb, sa * da);

    pipeline.r[i] = sr * (1.0f - da) + dr * (1.0f - sa) + rr;
    pipeline.g[i] = sg * (1.0f - da) + dg * (1.0f - sa) + gg;
    pipeline.b[i] = sb * (1.0f - da) + db * (1.0f - sa) + bb;
    pipeline.a[i] = sa + da - sa * da;
  }
  pipeline.nextStage();
}

// --- 2-point conical gradient stages ---

void xy_to_2pt_conical_focal_on_circle(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = (x == 0.0f) ? 0.0f : x + y * y / x;
  }
  pipeline.nextStage();
}

void xy_to_2pt_conical_well_behaved(Pipeline& pipeline) {
  const float p0 = pipeline.ctx->two_point_conical_gradient.p0;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = std::sqrt(x * x + y * y) - x * p0;
  }
  pipeline.nextStage();
}

void xy_to_2pt_conical_smaller(Pipeline& pipeline) {
  const float p0 = pipeline.ctx->two_point_conical_gradient.p0;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = -std::sqrt(x * x - y * y) - x * p0;
  }
  pipeline.nextStage();
}

void xy_to_2pt_conical_greater(Pipeline& pipeline) {
  const float p0 = pipeline.ctx->two_point_conical_gradient.p0;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = std::sqrt(x * x - y * y) - x * p0;
  }
  pipeline.nextStage();
}

void xy_to_2pt_conical_strip(Pipeline& pipeline) {
  const float p0 = pipeline.ctx->two_point_conical_gradient.p0;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float x = pipeline.r[i];
    const float y = pipeline.g[i];
    pipeline.r[i] = x + std::sqrt(p0 - y * y);
  }
  pipeline.nextStage();
}

void mask_2pt_conical_nan(Pipeline& pipeline) {
  auto& ctx = pipeline.ctx->two_point_conical_gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    const bool is_nan = (t != t);
    pipeline.r[i] = is_nan ? 0.0f : t;
    ctx.mask[i] = is_nan ? 0u : 0xFFFFFFFFu;
  }
  pipeline.nextStage();
}

void mask_2pt_conical_degenerates(Pipeline& pipeline) {
  auto& ctx = pipeline.ctx->two_point_conical_gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    const float t = pipeline.r[i];
    const bool is_degenerate = (t <= 0.0f) || (t != t);
    pipeline.r[i] = is_degenerate ? 0.0f : t;
    ctx.mask[i] = is_degenerate ? 0u : 0xFFFFFFFFu;
  }
  pipeline.nextStage();
}

void apply_vector_mask(Pipeline& pipeline) {
  const auto& mask = pipeline.ctx->two_point_conical_gradient.mask;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = float_and_mask(pipeline.r[i], mask[i]);
    pipeline.g[i] = float_and_mask(pipeline.g[i], mask[i]);
    pipeline.b[i] = float_and_mask(pipeline.b[i], mask[i]);
    pipeline.a[i] = float_and_mask(pipeline.a[i], mask[i]);
  }
  pipeline.nextStage();
}

void alter_2pt_conical_compensate_focal(Pipeline& pipeline) {
  const float p1 = pipeline.ctx->two_point_conical_gradient.p1;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] += p1;
  }
  pipeline.nextStage();
}

void alter_2pt_conical_unswap(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = 1.0f - pipeline.r[i];
  }
  pipeline.nextStage();
}

void negate_x(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = -pipeline.r[i];
  }
  pipeline.nextStage();
}

void apply_concentric_scale_bias(Pipeline& pipeline) {
  const auto& ctx = pipeline.ctx->two_point_conical_gradient;
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = pipeline.r[i] * ctx.p0 + ctx.p1;
  }
  pipeline.nextStage();
}

// --- Gamma correction stages ---

void gamma_expand_2(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] *= pipeline.r[i];
    pipeline.g[i] *= pipeline.g[i];
    pipeline.b[i] *= pipeline.b[i];
  }
  pipeline.nextStage();
}

void gamma_expand_dst_2(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.dr[i] *= pipeline.dr[i];
    pipeline.dg[i] *= pipeline.dg[i];
    pipeline.db[i] *= pipeline.db[i];
  }
  pipeline.nextStage();
}

void gamma_compress_2(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = std::sqrt(pipeline.r[i]);
    pipeline.g[i] = std::sqrt(pipeline.g[i]);
    pipeline.b[i] = std::sqrt(pipeline.b[i]);
  }
  pipeline.nextStage();
}

void gamma_expand_22(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = approxPowf(pipeline.r[i], 2.2f);
    pipeline.g[i] = approxPowf(pipeline.g[i], 2.2f);
    pipeline.b[i] = approxPowf(pipeline.b[i], 2.2f);
  }
  pipeline.nextStage();
}

void gamma_expand_dst_22(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.dr[i] = approxPowf(pipeline.dr[i], 2.2f);
    pipeline.dg[i] = approxPowf(pipeline.dg[i], 2.2f);
    pipeline.db[i] = approxPowf(pipeline.db[i], 2.2f);
  }
  pipeline.nextStage();
}

void gamma_compress_22(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = approxPowf(pipeline.r[i], 0.45454545f);
    pipeline.g[i] = approxPowf(pipeline.g[i], 0.45454545f);
    pipeline.b[i] = approxPowf(pipeline.b[i], 0.45454545f);
  }
  pipeline.nextStage();
}

void gamma_expand_srgb(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = srgb_expand_scalar(pipeline.r[i]);
    pipeline.g[i] = srgb_expand_scalar(pipeline.g[i]);
    pipeline.b[i] = srgb_expand_scalar(pipeline.b[i]);
  }
  pipeline.nextStage();
}

void gamma_expand_dst_srgb(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.dr[i] = srgb_expand_scalar(pipeline.dr[i]);
    pipeline.dg[i] = srgb_expand_scalar(pipeline.dg[i]);
    pipeline.db[i] = srgb_expand_scalar(pipeline.db[i]);
  }
  pipeline.nextStage();
}

void gamma_compress_srgb(Pipeline& pipeline) {
  for (std::size_t i = 0; i < kStageWidth; ++i) {
    pipeline.r[i] = srgb_compress_scalar(pipeline.r[i]);
    pipeline.g[i] = srgb_compress_scalar(pipeline.g[i]);
    pipeline.b[i] = srgb_compress_scalar(pipeline.b[i]);
  }
  pipeline.nextStage();
}

}  // namespace

void justReturn(Pipeline& pipeline) { (void)pipeline; }

void start(const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& functions,
           const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& tail_functions,
           const ScreenIntRect& rect, const AAMaskCtx& aa_mask_ctx, const MaskCtx& mask_ctx,
           Context& ctx, const PixmapRef& pixmap_src, SubPixmapMut* pixmap_dst) {
  Pipeline p(functions, tail_functions, rect, aa_mask_ctx, mask_ctx, ctx, pixmap_src, pixmap_dst);

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
    clamp_0,
    clamp_a,
    premultiply,
    uniform_color,
    seed_shader,
    load_dst,
    store,
    load_dst_u8,
    store_u8,
    gather,
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
    color_burn,
    color_dodge,
    darken,
    difference,
    exclusion,
    hard_light,
    lighten,
    overlay,
    soft_light,
    hue,
    saturation,
    color,
    luminosity,
    source_over_rgba,
    transform,
    reflect,
    repeat,
    bilinear,
    bicubic,
    pad_x1,
    reflect_x1,
    repeat_x1,
    gradient,
    evenly_spaced_2_stop_gradient,
    xy_to_unit_angle,
    xy_to_radius,
    xy_to_2pt_conical_focal_on_circle,
    xy_to_2pt_conical_well_behaved,
    xy_to_2pt_conical_smaller,
    xy_to_2pt_conical_greater,
    xy_to_2pt_conical_strip,
    mask_2pt_conical_nan,
    mask_2pt_conical_degenerates,
    apply_vector_mask,
    alter_2pt_conical_compensate_focal,
    alter_2pt_conical_unswap,
    negate_x,
    apply_concentric_scale_bias,
    gamma_expand_2,
    gamma_expand_dst_2,
    gamma_compress_2,
    gamma_expand_22,
    gamma_expand_dst_22,
    gamma_compress_22,
    gamma_expand_srgb,
    gamma_expand_dst_srgb,
    gamma_compress_srgb,
};

}  // namespace tiny_skia::pipeline::highp
