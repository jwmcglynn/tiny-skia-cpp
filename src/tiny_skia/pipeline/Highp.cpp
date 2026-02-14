#include "tiny_skia/pipeline/Highp.h"
#include "tiny_skia/Geom.h"

#include <algorithm>

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
           const ScreenIntRect& rect_arg,
           const AAMaskCtx& aa_mask_ctx_arg,
           const MaskCtx& mask_ctx_arg,
           Context& ctx_arg,
           const PixmapRef& pixmap_src_arg,
           SubPixmapMut* pixmap_dst_arg)
      : functions(&fun),
        rect(&rect_arg),
        aa_mask_ctx(&aa_mask_ctx_arg),
        mask_ctx(&mask_ctx_arg),
        ctx(&ctx_arg),
        pixmap_src(&pixmap_src_arg),
        pixmap_dst(pixmap_dst_arg) {
    (void)rect;
    (void)aa_mask_ctx;
    (void)mask_ctx;
    (void)ctx;
    (void)pixmap_src;
    (void)pixmap_dst;
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
    pipeline.r[i] = pipeline.r[i] * inv_da + pipeline.dr[i] * inv_sa + pipeline.r[i] * pipeline.dr[i];
    pipeline.g[i] = pipeline.g[i] * inv_da + pipeline.dg[i] * inv_sa + pipeline.g[i] * pipeline.dg[i];
    pipeline.b[i] = pipeline.b[i] * inv_da + pipeline.db[i] * inv_sa + pipeline.b[i] * pipeline.db[i];
    pipeline.a[i] = pipeline.a[i] * inv_da + pipeline.da[i] * inv_sa + pipeline.a[i] * pipeline.da[i];
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

#define STAGE_FN(name) void name(Pipeline& pipeline) { pipeline.nextStage(); }

STAGE_FN(load_dst)
STAGE_FN(store)
STAGE_FN(load_dst_u8)
STAGE_FN(store_u8)
STAGE_FN(gather)
STAGE_FN(load_mask_u8)
STAGE_FN(mask_u8)
STAGE_FN(darken)
STAGE_FN(difference)
STAGE_FN(exclusion)
STAGE_FN(hard_light)
STAGE_FN(lighten)
STAGE_FN(overlay)
STAGE_FN(soft_light)
STAGE_FN(hue)
STAGE_FN(saturation)
STAGE_FN(color)
STAGE_FN(luminosity)
STAGE_FN(source_over_rgba)
STAGE_FN(transform)
STAGE_FN(reflect)
STAGE_FN(repeat)
STAGE_FN(bilinear)
STAGE_FN(bicubic)
STAGE_FN(pad_x1)
STAGE_FN(reflect_x1)
STAGE_FN(repeat_x1)
STAGE_FN(gradient)
STAGE_FN(evenly_spaced_2_stop_gradient)
STAGE_FN(xy_to_unit_angle)
STAGE_FN(xy_to_radius)
STAGE_FN(xy_to_2pt_conical_focal_on_circle)
STAGE_FN(xy_to_2pt_conical_well_behaved)
STAGE_FN(xy_to_2pt_conical_smaller)
STAGE_FN(xy_to_2pt_conical_greater)
STAGE_FN(xy_to_2pt_conical_strip)
STAGE_FN(mask_2pt_conical_nan)
STAGE_FN(mask_2pt_conical_degenerates)
STAGE_FN(apply_vector_mask)
STAGE_FN(alter_2pt_conical_compensate_focal)
STAGE_FN(alter_2pt_conical_unswap)
STAGE_FN(negate_x)
STAGE_FN(apply_concentric_scale_bias)
STAGE_FN(gamma_expand_2)
STAGE_FN(gamma_expand_dst_2)
STAGE_FN(gamma_compress_2)
STAGE_FN(gamma_expand_22)
STAGE_FN(gamma_expand_dst_22)
STAGE_FN(gamma_compress_22)
STAGE_FN(gamma_expand_srgb)
STAGE_FN(gamma_expand_dst_srgb)
STAGE_FN(gamma_compress_srgb)

#undef STAGE_FN

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
           const PixmapRef& pixmap_src,
           SubPixmapMut* pixmap_dst) {
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
