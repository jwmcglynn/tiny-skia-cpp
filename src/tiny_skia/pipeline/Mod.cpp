#include "tiny_skia/pipeline/Mod.h"

#include <algorithm>
#include <cmath>

#include "tiny_skia/Color.h"
#include "tiny_skia/Math.h"
#include "tiny_skia/Point.h"
#include "tiny_skia/pipeline/Highp.h"
#include "tiny_skia/pipeline/Lowp.h"

namespace tiny_skia {

// ---------------------------------------------------------------------------
// Transform implementation (matches tiny_skia_path::Transform)
// ---------------------------------------------------------------------------

namespace {

double dcross(double a, double b, double c, double d) {
  return a * b - c * d;
}

float dcrossDscale(float a, float b, float c, float d, double scale) {
  return static_cast<float>(dcross(static_cast<double>(a), static_cast<double>(b),
                                   static_cast<double>(c), static_cast<double>(d)) * scale);
}

float mulAddMul(float a, float b, float c, float d) {
  return static_cast<float>(static_cast<double>(a) * static_cast<double>(b) +
                            static_cast<double>(c) * static_cast<double>(d));
}

Transform concat(const Transform& a, const Transform& b) {
  if (a.isIdentity()) return b;
  if (b.isIdentity()) return a;

  if (!a.hasSkew() && !b.hasSkew()) {
    return Transform::fromRow(
        a.sx * b.sx, 0.0f, 0.0f, a.sy * b.sy,
        a.sx * b.tx + a.tx, a.sy * b.ty + a.ty);
  }

  return Transform::fromRow(
      mulAddMul(a.sx, b.sx, a.kx, b.ky),
      mulAddMul(a.ky, b.sx, a.sy, b.ky),
      mulAddMul(a.sx, b.kx, a.kx, b.sy),
      mulAddMul(a.ky, b.kx, a.sy, b.sy),
      mulAddMul(a.sx, b.tx, a.kx, b.ty) + a.tx,
      mulAddMul(a.ky, b.tx, a.sy, b.ty) + a.ty);
}

std::optional<double> invDeterminant(const Transform& ts) {
  const auto det = dcross(static_cast<double>(ts.sx), static_cast<double>(ts.sy),
                          static_cast<double>(ts.kx), static_cast<double>(ts.ky));
  const float tolerance = kScalarNearlyZero * kScalarNearlyZero * kScalarNearlyZero;
  if (isNearlyZeroWithinTolerance(static_cast<float>(det), tolerance)) {
    return std::nullopt;
  }
  return 1.0 / det;
}

Transform computeInv(const Transform& ts, double invDet) {
  return Transform::fromRow(
      static_cast<float>(static_cast<double>(ts.sy) * invDet),
      static_cast<float>(static_cast<double>(-ts.ky) * invDet),
      static_cast<float>(static_cast<double>(-ts.kx) * invDet),
      static_cast<float>(static_cast<double>(ts.sx) * invDet),
      dcrossDscale(ts.kx, ts.ty, ts.sy, ts.tx, invDet),
      dcrossDscale(ts.ky, ts.tx, ts.sx, ts.ty, invDet));
}

}  // namespace

bool Transform::isFinite() const {
  return std::isfinite(sx) && std::isfinite(ky) && std::isfinite(kx) &&
         std::isfinite(sy) && std::isfinite(tx) && std::isfinite(ty);
}

bool Transform::isIdentity() const {
  return *this == Transform::identity();
}

bool Transform::isTranslate() const {
  return !hasScale() && !hasSkew();
}

bool Transform::isScaleTranslate() const {
  return !hasSkew();
}

bool Transform::hasScale() const {
  return sx != 1.0f || sy != 1.0f;
}

bool Transform::hasSkew() const {
  return kx != 0.0f || ky != 0.0f;
}

bool Transform::hasTranslate() const {
  return tx != 0.0f || ty != 0.0f;
}

std::optional<Transform> Transform::invert() const {
  if (isIdentity()) return *this;

  if (isScaleTranslate()) {
    if (hasScale()) {
      const auto invX = tiny_skia::invert(sx);
      const auto invY = tiny_skia::invert(sy);
      if (!std::isfinite(invX) || !std::isfinite(invY)) {
        return std::nullopt;
      }
      return fromRow(invX, 0.0f, 0.0f, invY, -tx * invX, -ty * invY);
    }
    return fromTranslate(-tx, -ty);
  }

  const auto invDet = invDeterminant(*this);
  if (!invDet.has_value()) return std::nullopt;

  const auto invTs = computeInv(*this, *invDet);
  if (!invTs.isFinite()) return std::nullopt;

  return invTs;
}

Transform Transform::preConcat(const Transform& other) const {
  return concat(*this, other);
}

Transform Transform::postConcat(const Transform& other) const {
  return concat(other, *this);
}

Transform Transform::preScale(float sx, float sy) const {
  return preConcat(fromScale(sx, sy));
}

Transform Transform::postScale(float sx, float sy) const {
  return postConcat(fromScale(sx, sy));
}

Transform Transform::preTranslate(float tx, float ty) const {
  return preConcat(fromTranslate(tx, ty));
}

Transform Transform::postTranslate(float tx, float ty) const {
  return postConcat(fromTranslate(tx, ty));
}

void Transform::mapPoints(std::span<Point> points) const {
  for (auto& p : points) {
    const float nx = sx * p.x + kx * p.y + tx;
    const float ny = ky * p.x + sy * p.y + ty;
    p.x = nx;
    p.y = ny;
  }
}

}  // namespace tiny_skia

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
  return false;  // unreachable; satisfies -Wreturn-type
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

RasterPipeline::RasterPipeline(Kind kind,
                             Context context,
                             const std::array<Stage, kMaxStages>& stages,
                             std::size_t stage_count) {
  stage_count_ = std::min(stage_count, kMaxStages);
  kind_ = kind;
  ctx_ = context;
  for (std::size_t i = 0; i < kMaxStages; ++i) {
    stages_[i] = Stage::MoveDestinationToSource;
  }
  for (std::size_t i = 0; i < stage_count_; ++i) {
    stages_[i] = stages[i];
  }
}

namespace {

[[nodiscard]] std::array<highp::StageFn, kMaxStages> highpFunctions(
    const std::array<Stage, kMaxStages>& stages,
    std::size_t stage_count) {
  std::array<highp::StageFn, kMaxStages> functions{};
  std::fill(functions.begin(), functions.end(), &highp::justReturn);

  for (std::size_t i = 0; i < stage_count && i < kMaxStages; ++i) {
    functions[i] = highp::STAGES[static_cast<std::size_t>(stages[i])];
  }
  return functions;
}

[[nodiscard]] std::array<highp::StageFn, kMaxStages> highpTailFunctions(
    const std::array<Stage, kMaxStages>& stages,
    std::size_t stage_count) {
  std::array<highp::StageFn, kMaxStages> tail_functions{};
  std::fill(tail_functions.begin(), tail_functions.end(), &highp::justReturn);

  for (std::size_t i = 0; i < stage_count && i < kMaxStages; ++i) {
    tail_functions[i] = highp::STAGES[static_cast<std::size_t>(stages[i])];
  }
  return tail_functions;
}

[[nodiscard]] std::array<lowp::StageFn, kMaxStages> lowpFunctions(
    const std::array<Stage, kMaxStages>& stages,
    std::size_t stage_count) {
  std::array<lowp::StageFn, kMaxStages> functions{};
  std::fill(functions.begin(), functions.end(), &lowp::justReturn);

  for (std::size_t i = 0; i < stage_count && i < kMaxStages; ++i) {
    functions[i] = lowp::STAGES[static_cast<std::size_t>(stages[i])];
  }
  return functions;
}

[[nodiscard]] std::array<lowp::StageFn, kMaxStages> lowpTailFunctions(
    const std::array<Stage, kMaxStages>& stages,
    std::size_t stage_count) {
  std::array<lowp::StageFn, kMaxStages> tail_functions{};
  std::fill(tail_functions.begin(), tail_functions.end(), &lowp::justReturn);

  for (std::size_t i = 0; i < stage_count && i < kMaxStages; ++i) {
    tail_functions[i] = lowp::STAGES[static_cast<std::size_t>(stages[i])];
  }
  return tail_functions;
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
    RasterPipeline pipeline(RasterPipeline::Kind::High, Context{}, stages_, 0);
    return pipeline;
  }

  const bool is_lowp_compatible = isPipelineLowpCompatible(stages_, stage_count_);
  const auto kind = (force_hq_pipeline_ || !is_lowp_compatible)
                        ? RasterPipeline::Kind::High
                        : RasterPipeline::Kind::Low;
  return RasterPipeline(kind, ctx_, stages_, stage_count_);
}

void RasterPipeline::run(const ScreenIntRect& rect,
                        const AAMaskCtx& aa_mask_ctx,
                        MaskCtx mask_ctx,
                        const PixmapRef& pixmap_src,
                        SubPixmapMut* pixmap_dst) {
  if (stage_count_ == 0) {
    return;
  }

  const auto highp_functions = highpFunctions(stages_, stage_count_);
  const auto highp_tail_functions = highpTailFunctions(stages_, stage_count_);
  const auto lowp_functions = lowpFunctions(stages_, stage_count_);
  const auto lowp_tail_functions = lowpTailFunctions(stages_, stage_count_);

  switch (kind_) {
    case Kind::High:
      highp::start(highp_functions,
                   highp_tail_functions,
                   rect,
                   aa_mask_ctx,
                   mask_ctx,
                   ctx_,
                   pixmap_src,
                   pixmap_dst);
      break;
    case Kind::Low:
      lowp::start(lowp_functions,
                   lowp_tail_functions,
                   rect,
                   aa_mask_ctx,
                   mask_ctx,
                   ctx_,
                   pixmap_dst);
      break;
  }
}

}  // namespace tiny_skia::pipeline
