#include "tiny_skia/pipeline/Mod.h"

#include "tiny_skia/Color.h"

namespace tiny_skia::pipeline {

static_assert(kStagesCount == 79);
static_assert(sizeof(GradientColor) == sizeof(float) * 4);

namespace {

[[nodiscard]] constexpr bool isLowpCompatible(Stage stage) {
  switch (stage) {
    case Stage::Clamp0:
    case Stage::ClampA:
    case Stage::Gather:
    case Stage::ColorBurn:
    case Stage::ColorDodge:
    case Stage::SoftLight:
    case Stage::Hue:
    case Stage::Saturation:
    case Stage::Color:
    case Stage::Luminosity:
    case Stage::Reflect:
    case Stage::Repeat:
    case Stage::Bilinear:
    case Stage::Bicubic:
    case Stage::XYToUnitAngle:
    case Stage::XYTo2PtConicalFocalOnCircle:
    case Stage::XYTo2PtConicalWellBehaved:
    case Stage::XYTo2PtConicalSmaller:
    case Stage::XYTo2PtConicalGreater:
    case Stage::XYTo2PtConicalStrip:
    case Stage::Mask2PtConicalNan:
    case Stage::Mask2PtConicalDegenerates:
    case Stage::ApplyVectorMask:
    case Stage::Alter2PtConicalCompensateFocal:
    case Stage::Alter2PtConicalUnswap:
    case Stage::NegateX:
    case Stage::ApplyConcentricScaleBias:
    case Stage::GammaExpand2:
    case Stage::GammaExpandDestination2:
    case Stage::GammaCompress2:
    case Stage::GammaExpand22:
    case Stage::GammaExpandDestination22:
    case Stage::GammaCompress22:
    case Stage::GammaExpandSrgb:
    case Stage::GammaExpandDestinationSrgb:
    case Stage::GammaCompressSrgb:
      return false;
    case Stage::MoveSourceToDestination:
    case Stage::MoveDestinationToSource:
    case Stage::Premultiply:
    case Stage::UniformColor:
    case Stage::SeedShader:
    case Stage::LoadDestination:
    case Stage::Store:
    case Stage::LoadDestinationU8:
    case Stage::StoreU8:
    case Stage::LoadMaskU8:
    case Stage::MaskU8:
    case Stage::ScaleU8:
    case Stage::LerpU8:
    case Stage::Scale1Float:
    case Stage::Lerp1Float:
    case Stage::DestinationAtop:
    case Stage::DestinationIn:
    case Stage::DestinationOut:
    case Stage::DestinationOver:
    case Stage::SourceAtop:
    case Stage::SourceIn:
    case Stage::SourceOut:
    case Stage::SourceOver:
    case Stage::Clear:
    case Stage::Modulate:
    case Stage::Multiply:
    case Stage::Plus:
    case Stage::Screen:
    case Stage::Xor:
    case Stage::Darken:
    case Stage::Difference:
    case Stage::Exclusion:
    case Stage::HardLight:
    case Stage::Lighten:
    case Stage::Overlay:
    case Stage::SourceOverRgba:
    case Stage::Transform:
    case Stage::PadX1:
    case Stage::ReflectX1:
    case Stage::RepeatX1:
    case Stage::Gradient:
    case Stage::EvenlySpaced2StopGradient:
    case Stage::XYToRadius:
      return true;
  }
}

[[nodiscard]] constexpr bool isPipelineLowpCompatible(
    const std::array<Stage, kMaxStages>& stages,
    std::size_t count) {
  for (std::size_t i = 0; i < count; ++i) {
    if (!isLowpCompatible(stages[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

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

  const bool is_lowp_compatible = isPipelineLowpCompatible(stages_, stage_count_);
  const auto kind = (force_hq_pipeline_ || !is_lowp_compatible)
                        ? RasterPipeline::Kind::High
                        : RasterPipeline::Kind::Low;
  return RasterPipeline(kind, ctx_);
}

}  // namespace tiny_skia::pipeline
