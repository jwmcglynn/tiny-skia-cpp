#include "tiny_skia/pipeline/Mod.h"

#include "tiny_skia/Color.h"

namespace tiny_skia::pipeline {

static_assert(kStagesCount == 79);
static_assert(sizeof(GradientColor) == sizeof(float) * 4);

void RasterPipelineBuilder::pushUniformColor(const PremultipliedColor& color) {
  const auto r = color.red();
  const auto g = color.green();
  const auto b = color.blue();
  const auto a = color.alpha();
  const auto rgba = std::array<std::uint16_t, 4>{
      static_cast<std::uint16_t>(r * 255.0f + 0.5f),
      static_cast<std::uint16_t>(g * 255.0f + 0.5f),
      static_cast<std::uint16_t>(b * 255.0f + 0.5f),
      static_cast<std::uint16_t>(a * 255.0f + 0.5f),
  };

  ctx_.uniform_color = UniformColorCtx{
      .r = r,
      .g = g,
      .b = b,
      .a = a,
      .rgba = rgba,
  };
  push(Stage::UniformColor);
}

RasterPipeline RasterPipelineBuilder::compile() {
  if (stage_count_ == 0) {
    RasterPipeline pipeline(RasterPipeline::Kind::High, Context{});
    return pipeline;
  }

  const auto kind = (force_hq_pipeline_) ? RasterPipeline::Kind::High : RasterPipeline::Kind::Low;
  return RasterPipeline(kind, ctx_);
}

}  // namespace tiny_skia::pipeline
