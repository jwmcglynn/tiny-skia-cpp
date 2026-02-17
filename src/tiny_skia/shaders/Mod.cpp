#include "tiny_skia/shaders/Mod.h"

#include <cmath>

#include "tiny_skia/Math.h"

namespace tiny_skia {

// ---------------------------------------------------------------------------
// Gradient
// ---------------------------------------------------------------------------

Gradient::Gradient(std::vector<GradientStop> stops, SpreadMode tileMode,
                   Transform transform, Transform pointsToUnit)
    : transform(transform),
      stops_(std::move(stops)),
      tile_mode_(tileMode),
      points_to_unit_(pointsToUnit) {
  // Insert dummy endpoints if needed (matching Rust Gradient::new).
  const bool dummyFirst = stops_[0].position.get() != 0.0f;
  const bool dummyLast = stops_[stops_.size() - 1].position.get() != 1.0f;

  if (dummyFirst) {
    stops_.insert(stops_.begin(), GradientStop::create(0.0f, stops_[0].color));
  }
  if (dummyLast) {
    stops_.push_back(GradientStop::create(1.0f, stops_[stops_.size() - 1].color));
  }

  colors_are_opaque_ = true;
  for (const auto& s : stops_) {
    if (!s.color.isOpaque()) {
      colors_are_opaque_ = false;
      break;
    }
  }

  // Ensure monotonic positions and check for uniform stops.
  const std::size_t startIndex = dummyFirst ? 0 : 1;
  float prev = 0.0f;
  has_uniform_stops_ = true;
  const float uniformStep = stops_[startIndex].position.get() - prev;

  for (std::size_t i = startIndex; i < stops_.size(); ++i) {
    float curr;
    if (i + 1 == stops_.size()) {
      curr = 1.0f;
    } else {
      curr = bound(prev, stops_[i].position.get(), 1.0f);
    }
    has_uniform_stops_ &= isNearlyEqual(uniformStep, curr - prev);
    stops_[i].position = NormalizedF32::newClamped(curr);
    prev = curr;
  }
}

bool Gradient::pushStages(
    pipeline::RasterPipelineBuilder& p, ColorSpace cs,
    const std::function<void(pipeline::RasterPipelineBuilder&)>& pushStagesPre,
    const std::function<void(pipeline::RasterPipelineBuilder&)>& pushStagesPost) const {
  using Stage = pipeline::Stage;

  p.push(Stage::SeedShader);

  const auto ts = transform.invert();
  if (!ts.has_value()) {
    return false;
  }
  const auto finalTs = ts->postConcat(points_to_unit_);
  p.pushTransform(finalTs);

  pushStagesPre(p);

  switch (tile_mode_) {
    case SpreadMode::Reflect:
      p.push(Stage::ReflectX1);
      break;
    case SpreadMode::Repeat:
      p.push(Stage::RepeatX1);
      break;
    case SpreadMode::Pad:
      if (has_uniform_stops_) {
        p.push(Stage::PadX1);
      }
      break;
  }

  // Two-stop optimization.
  if (stops_.size() == 2) {
    const auto c0 = expandColor(cs, stops_[0].color);
    const auto c1 = expandColor(cs, stops_[1].color);

    p.ctx().evenly_spaced_2_stop_gradient = pipeline::EvenlySpaced2StopGradientCtx{
        .factor = pipeline::GradientColor{
            c1.red() - c0.red(),
            c1.green() - c0.green(),
            c1.blue() - c0.blue(),
            c1.alpha() - c0.alpha()},
        .bias = pipeline::GradientColor{c0.red(), c0.green(), c0.blue(), c0.alpha()}};

    p.push(Stage::EvenlySpaced2StopGradient);
  } else {
    pipeline::Context::GradientCtx ctx;

    ctx.factors.reserve(std::max(stops_.size() + 1, std::size_t{16}));
    ctx.biases.reserve(std::max(stops_.size() + 1, std::size_t{16}));
    ctx.t_values.reserve(stops_.size() + 1);

    // Remove dummy stops for the search (matching Rust logic).
    std::size_t firstStop, lastStop;
    if (stops_.size() > 2) {
      firstStop = (stops_[0].color != stops_[1].color) ? 0 : 1;
      const auto len = stops_.size();
      lastStop = (stops_[len - 2].color != stops_[len - 1].color) ? len - 1 : len - 2;
    } else {
      firstStop = 0;
      lastStop = 1;
    }

    float tL = stops_[firstStop].position.get();
    auto cL = ([&] {
      const auto c = expandColor(cs, stops_[firstStop].color);
      return pipeline::GradientColor{c.red(), c.green(), c.blue(), c.alpha()};
    }());
    ctx.pushConstColor(cL);
    ctx.t_values.push_back(0.0f);

    for (std::size_t i = firstStop; i < lastStop; ++i) {
      const float tR = stops_[i + 1].position.get();
      const auto cExpanded = expandColor(cs, stops_[i + 1].color);
      const auto cR = pipeline::GradientColor{
          cExpanded.red(), cExpanded.green(), cExpanded.blue(), cExpanded.alpha()};

      if (tL < tR) {
        const float invDt = 1.0f / (tR - tL);
        const auto f = pipeline::GradientColor{
            (cR.r - cL.r) * invDt, (cR.g - cL.g) * invDt,
            (cR.b - cL.b) * invDt, (cR.a - cL.a) * invDt};
        ctx.factors.push_back(f);
        ctx.biases.push_back(pipeline::GradientColor{
            cL.r - f.r * tL, cL.g - f.g * tL,
            cL.b - f.b * tL, cL.a - f.a * tL});
        ctx.t_values.push_back(bound(0.0f, tL, 1.0f));
      }

      tL = tR;
      cL = cR;
    }

    ctx.pushConstColor(cL);
    ctx.t_values.push_back(bound(0.0f, tL, 1.0f));

    ctx.len = ctx.factors.size();

    // Pad to 16 for lowp F32x16 alignment.
    while (ctx.factors.size() < 16) {
      ctx.factors.push_back(pipeline::GradientColor{});
      ctx.biases.push_back(pipeline::GradientColor{});
    }

    p.push(Stage::Gradient);
    p.ctx().gradient = std::move(ctx);
  }

  if (!colors_are_opaque_) {
    p.push(Stage::Premultiply);
  }

  pushStagesPost(p);

  return true;
}

void Gradient::applyOpacity(float opacity) {
  for (auto& stop : stops_) {
    stop.color.applyOpacity(opacity);
  }
  colors_are_opaque_ = true;
  for (const auto& s : stops_) {
    if (!s.color.isOpaque()) {
      colors_are_opaque_ = false;
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// LinearGradient
// ---------------------------------------------------------------------------

namespace {

float sdot(float a, float b, float c, float d) {
  return a * b + c * d;
}

Transform tsFromSinCosAt(float sin, float cos, float px, float py) {
  const float cosInv = 1.0f - cos;
  return Transform::fromRow(
      cos, sin, -sin, cos,
      sdot(sin, py, cosInv, px),
      sdot(-sin, px, cosInv, py));
}

std::optional<Transform> pointsToUnitTs(Point start, Point end) {
  auto vec = end - start;
  const float mag = vec.length();
  const float inv = (mag != 0.0f) ? tiny_skia::invert(mag) : 0.0f;

  vec.scale(inv);

  auto ts = tsFromSinCosAt(-vec.y, vec.x, start.x, start.y);
  ts = ts.postTranslate(-start.x, -start.y);
  ts = ts.postScale(inv, inv);
  return ts;
}

Color averageGradientColor(const std::vector<GradientStop>& stops) {
  // Simplified average: accumulate weighted color contributions.
  float blendR = 0.0f, blendG = 0.0f, blendB = 0.0f, blendA = 0.0f;

  for (std::size_t i = 0; i + 1 < stops.size(); ++i) {
    const auto c0 = stops[i].color;
    const auto c1 = stops[i + 1].color;
    const float w = 0.5f * (stops[i + 1].position.get() - stops[i].position.get());
    blendR += w * (c0.red() + c1.red());
    blendG += w * (c0.green() + c1.green());
    blendB += w * (c0.blue() + c1.blue());
    blendA += w * (c0.alpha() + c1.alpha());
  }

  // Account for implicit interval at start.
  if (stops[0].position.get() > 0.0f) {
    const float p = stops[0].position.get();
    blendR += p * stops[0].color.red();
    blendG += p * stops[0].color.green();
    blendB += p * stops[0].color.blue();
    blendA += p * stops[0].color.alpha();
  }

  // Account for implicit interval at end.
  const auto lastIdx = stops.size() - 1;
  if (stops[lastIdx].position.get() < 1.0f) {
    const float p = 1.0f - stops[lastIdx].position.get();
    blendR += p * stops[lastIdx].color.red();
    blendG += p * stops[lastIdx].color.green();
    blendB += p * stops[lastIdx].color.blue();
    blendA += p * stops[lastIdx].color.alpha();
  }

  return Color::fromRgba(blendR, blendG, blendB, blendA).value_or(Color::transparent);
}

}  // namespace

// ---------------------------------------------------------------------------
// Transform helpers for radial gradients (not in anon namespace — used by
// FocalData::set which lives in tiny_skia namespace).
// ---------------------------------------------------------------------------

namespace {

Transform fromPoly2(Point p0, Point p1) {
  return Transform::fromRow(
      p1.y - p0.y,
      p0.x - p1.x,
      p1.x - p0.x,
      p1.y - p0.y,
      p0.x,
      p0.y);
}

}  // namespace

std::optional<Transform> tsFromPolyToPoly(Point src1, Point src2,
                                          Point dst1, Point dst2) {
  const auto tmp = fromPoly2(src1, src2);
  const auto res = tmp.invert();
  if (!res.has_value()) return std::nullopt;
  const auto tmp2 = fromPoly2(dst1, dst2);
  return tmp2.preConcat(*res);
}

namespace {

std::optional<Transform> mapToUnitX(Point origin, Point xIsOne) {
  return tsFromPolyToPoly(
      origin, xIsOne,
      Point::fromXy(0.0f, 0.0f), Point::fromXy(1.0f, 0.0f));
}

}  // namespace

std::optional<std::variant<Color, LinearGradient>> LinearGradient::create(
    Point start, Point end, std::vector<GradientStop> stops,
    SpreadMode mode, Transform transform) {
  if (stops.empty()) return std::nullopt;

  if (stops.size() == 1) {
    return std::variant<Color, LinearGradient>{stops[0].color};
  }

  const float length = (end - start).length();
  if (!std::isfinite(length)) return std::nullopt;

  if (isNearlyZeroWithinTolerance(length, kDegenerateThreshold)) {
    switch (mode) {
      case SpreadMode::Pad:
        return std::variant<Color, LinearGradient>{stops.back().color};
      case SpreadMode::Reflect:
      case SpreadMode::Repeat:
        return std::variant<Color, LinearGradient>{averageGradientColor(stops)};
    }
  }

  if (!transform.invert().has_value()) return std::nullopt;

  const auto unitTs = pointsToUnitTs(start, end);
  if (!unitTs.has_value()) return std::nullopt;

  return std::variant<Color, LinearGradient>{
      LinearGradient{Gradient(std::move(stops), mode, transform, *unitTs)}};
}

bool LinearGradient::pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const {
  return base_.pushStages(p, cs, [](auto&) {}, [](auto&) {});
}

// ---------------------------------------------------------------------------
// SweepGradient
// ---------------------------------------------------------------------------

std::optional<std::variant<Color, SweepGradient>> SweepGradient::create(
    Point center, float startAngle, float endAngle,
    std::vector<GradientStop> stops, SpreadMode mode, Transform transform) {
  if (!std::isfinite(startAngle) || !std::isfinite(endAngle) || startAngle > endAngle) {
    return std::nullopt;
  }

  if (stops.empty()) return std::nullopt;

  if (stops.size() == 1) {
    return std::variant<Color, SweepGradient>{stops[0].color};
  }

  if (!transform.invert().has_value()) return std::nullopt;

  if (isNearlyEqualWithinTolerance(startAngle, endAngle, kDegenerateThreshold)) {
    if (mode == SpreadMode::Pad && endAngle > kDegenerateThreshold) {
      const auto frontColor = stops.front().color;
      const auto backColor = stops.back().color;
      std::vector<GradientStop> newStops;
      newStops.push_back(GradientStop::create(0.0f, frontColor));
      newStops.push_back(GradientStop::create(1.0f, frontColor));
      newStops.push_back(GradientStop::create(1.0f, backColor));
      return SweepGradient::create(center, 0.0f, endAngle, std::move(newStops), mode, transform);
    }
    return std::nullopt;
  }

  if (startAngle <= 0.0f && endAngle >= 360.0f) {
    mode = SpreadMode::Pad;
  }

  const float t0 = startAngle / 360.0f;
  const float t1 = endAngle / 360.0f;

  SweepGradient sg{Gradient(std::move(stops), mode, transform,
                             Transform::fromTranslate(-center.x, -center.y))};
  sg.t0_ = t0;
  sg.t1_ = t1;
  return std::variant<Color, SweepGradient>{std::move(sg)};
}

bool SweepGradient::pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const {
  using Stage = pipeline::Stage;

  const float scale = 1.0f / (t1_ - t0_);
  const float bias = -scale * t0_;
  p.ctx().two_point_conical_gradient.p0 = scale;
  p.ctx().two_point_conical_gradient.p1 = bias;

  return base_.pushStages(p, cs,
      [scale, bias](pipeline::RasterPipelineBuilder& b) {
        b.push(Stage::XYToUnitAngle);
        if (scale != 1.0f || bias != 0.0f) {
          b.push(Stage::ApplyConcentricScaleBias);
        }
      },
      [](auto&) {});
}

// ---------------------------------------------------------------------------
// FocalData
// ---------------------------------------------------------------------------

bool FocalData::isFocalOnCircle() const {
  return isNearlyEqual(1.0f, r1);
}

bool FocalData::isWellBehaved() const {
  return !isFocalOnCircle() && r1 > 1.0f;
}

bool FocalData::isNativelyFocal() const {
  return isNearlyZero(focal_x);
}

bool FocalData::set(float r0_in, float r1_in, Transform& matrix) {
  is_swapped = false;
  focal_x = r0_in / (r0_in - r1_in);

  if (isNearlyEqual(focal_x, 1.0f)) {
    matrix = matrix.postTranslate(-1.0f, 0.0f).postScale(-1.0f, 1.0f);
    std::swap(r0_in, r1_in);
    focal_x = 0.0f;
    is_swapped = true;
  }

  const auto focalMatrix = tsFromPolyToPoly(
      Point::fromXy(focal_x, 0.0f), Point::fromXy(1.0f, 0.0f),
      Point::fromXy(0.0f, 0.0f), Point::fromXy(1.0f, 0.0f));
  if (!focalMatrix.has_value()) return false;

  matrix = matrix.postConcat(*focalMatrix);
  r1 = r1_in / std::abs(1.0f - focal_x);

  if (isFocalOnCircle()) {
    matrix = matrix.postScale(0.5f, 0.5f);
  } else {
    matrix = matrix.postScale(
        r1 / (r1 * r1 - 1.0f),
        1.0f / std::sqrt(std::abs(r1 * r1 - 1.0f)));
  }

  matrix = matrix.postScale(std::abs(1.0f - focal_x), std::abs(1.0f - focal_x));
  return true;
}

// ---------------------------------------------------------------------------
// RadialGradient
// ---------------------------------------------------------------------------

std::optional<std::variant<Color, RadialGradient>> RadialGradient::createRadialUnchecked(
    Point center, float radius, std::vector<GradientStop> stops,
    SpreadMode mode, Transform transform) {
  const float inv = (radius != 0.0f) ? tiny_skia::invert(radius) : 0.0f;
  if (!std::isfinite(inv)) return std::nullopt;
  const auto pointsToUnit = Transform::fromTranslate(-center.x, -center.y)
                                 .postScale(inv, inv);

  RadialGradient rg{Gradient(std::move(stops), mode, transform, pointsToUnit)};
  rg.gradient_type_ = RadialType{0.0f, radius};
  return std::variant<Color, RadialGradient>{std::move(rg)};
}

std::optional<std::variant<Color, RadialGradient>> RadialGradient::createTwoPoint(
    Point c0, float r0, Point c1, float r1,
    std::vector<GradientStop> stops, SpreadMode mode, Transform transform) {
  GradientType gradientType;
  Transform gradientMatrix;

  if ((c0 - c1).length() < kDegenerateThreshold) {
    // Concentric
    const float maxR = std::max(r0, r1);
    if (isNearlyZero(maxR) || isNearlyEqual(r0, r1)) {
      return std::nullopt;
    }
    const float scale = 1.0f / maxR;
    gradientMatrix = Transform::fromTranslate(-c1.x, -c1.y)
                         .postScale(scale, scale);
    gradientType = RadialType{r0, r1};
  } else {
    // Different centers
    const auto unitXTs = mapToUnitX(c0, c1);
    if (!unitXTs.has_value()) return std::nullopt;
    gradientMatrix = *unitXTs;

    const float dCenter = (c0 - c1).length();

    if (isNearlyZeroWithinTolerance(r0 - r1, kDegenerateThreshold)) {
      // Strip gradient (equal radii)
      const float scaledR0 = r0 / dCenter;
      gradientType = StripType{scaledR0};
    } else {
      // Focal gradient
      FocalData fd{};
      if (!fd.set(r0 / dCenter, r1 / dCenter, gradientMatrix)) {
        return std::nullopt;
      }
      gradientType = fd;
    }
  }

  RadialGradient rg{Gradient(std::move(stops), mode, transform, gradientMatrix)};
  rg.gradient_type_ = std::move(gradientType);
  return std::variant<Color, RadialGradient>{std::move(rg)};
}

std::optional<std::variant<Color, RadialGradient>> RadialGradient::create(
    Point startPoint, float startRadius, Point endPoint, float endRadius,
    std::vector<GradientStop> stops, SpreadMode mode, Transform transform) {
  if (startRadius < 0.0f || endRadius < 0.0f) return std::nullopt;

  if (stops.empty()) return std::nullopt;

  if (stops.size() == 1) {
    return std::variant<Color, RadialGradient>{stops[0].color};
  }

  if (!transform.invert().has_value()) return std::nullopt;

  const float length = (startPoint - endPoint).length();
  if (!std::isfinite(length)) return std::nullopt;

  if (isNearlyZeroWithinTolerance(length, kDegenerateThreshold)) {
    if (isNearlyEqualWithinTolerance(startRadius, endRadius, kDegenerateThreshold)) {
      // Both center and radii are the same — fully degenerate.
      if (mode == SpreadMode::Pad && endRadius > kDegenerateThreshold) {
        const auto startColor = stops.front().color;
        const auto endColor = stops.back().color;
        std::vector<GradientStop> newStops;
        newStops.push_back(GradientStop::create(0.0f, startColor));
        newStops.push_back(GradientStop::create(1.0f, startColor));
        newStops.push_back(GradientStop::create(1.0f, endColor));
        return createRadialUnchecked(startPoint, endRadius, std::move(newStops), mode, transform);
      }
      return std::nullopt;
    }

    // Same center, different radii.
    if (isNearlyZeroWithinTolerance(startRadius, kDegenerateThreshold)) {
      return createRadialUnchecked(startPoint, endRadius, std::move(stops), mode, transform);
    }
  }

  return createTwoPoint(startPoint, startRadius, endPoint, endRadius,
                         std::move(stops), mode, transform);
}

bool RadialGradient::pushStages(ColorSpace cs,
                                 pipeline::RasterPipelineBuilder& p) const {
  using Stage = pipeline::Stage;

  float p0 = 0.0f, p1 = 0.0f;
  if (const auto* rt = std::get_if<RadialType>(&gradient_type_)) {
    if (rt->radius1 == 0.0f) {
      p0 = 1.0f;
      p1 = 0.0f;
    } else {
      const float dRadius = rt->radius2 - rt->radius1;
      p0 = std::max(rt->radius1, rt->radius2) / dRadius;
      p1 = -rt->radius1 / dRadius;
    }
  } else if (const auto* st = std::get_if<StripType>(&gradient_type_)) {
    p0 = st->scaled_r0 * st->scaled_r0;
    p1 = 0.0f;
  } else if (const auto* fd = std::get_if<FocalData>(&gradient_type_)) {
    p0 = 1.0f / fd->r1;
    p1 = fd->focal_x;
  }

  p.ctx().two_point_conical_gradient = pipeline::TwoPointConicalGradientCtx{
      .mask = 0, .p0 = p0, .p1 = p1};

  const auto& gt = gradient_type_;
  return base_.pushStages(p, cs,
      [&gt, p0, p1](pipeline::RasterPipelineBuilder& b) {
        if (std::holds_alternative<RadialType>(gt)) {
          b.push(Stage::XYToRadius);
          if (p0 != 1.0f || p1 != 0.0f) {
            b.push(Stage::ApplyConcentricScaleBias);
          }
        } else if (std::holds_alternative<StripType>(gt)) {
          b.push(Stage::XYTo2PtConicalStrip);
          b.push(Stage::Mask2PtConicalNan);
        } else if (const auto* fd = std::get_if<FocalData>(&gt)) {
          if (fd->isFocalOnCircle()) {
            b.push(Stage::XYTo2PtConicalFocalOnCircle);
          } else if (fd->isWellBehaved()) {
            b.push(Stage::XYTo2PtConicalWellBehaved);
          } else if (fd->is_swapped || (1.0f - fd->focal_x) < 0.0f) {
            b.push(Stage::XYTo2PtConicalSmaller);
          } else {
            b.push(Stage::XYTo2PtConicalGreater);
          }

          if (!fd->isWellBehaved()) {
            b.push(Stage::Mask2PtConicalDegenerates);
          }

          if ((1.0f - fd->focal_x) < 0.0f) {
            b.push(Stage::NegateX);
          }

          if (!fd->isNativelyFocal()) {
            b.push(Stage::Alter2PtConicalCompensateFocal);
          }

          if (fd->is_swapped) {
            b.push(Stage::Alter2PtConicalUnswap);
          }
        }
      },
      [&gt](pipeline::RasterPipelineBuilder& b) {
        if (std::holds_alternative<StripType>(gt)) {
          b.push(Stage::ApplyVectorMask);
        } else if (const auto* fd = std::get_if<FocalData>(&gt)) {
          if (!fd->isWellBehaved()) {
            b.push(Stage::ApplyVectorMask);
          }
        }
      });
}

// ---------------------------------------------------------------------------
// Pattern
// ---------------------------------------------------------------------------

Pattern::Pattern(PixmapRef pixmap, SpreadMode spreadMode, FilterQuality quality,
                 float opacity, Transform transform)
    : pixmap_(pixmap),
      opacity_(NormalizedF32::newClamped(opacity)),
      transform_(transform),
      quality_(quality),
      spread_mode_(spreadMode) {}

bool Pattern::isOpaque() const {
  // Matches Rust: Pattern always reports as non-opaque.
  return false;
}

bool Pattern::pushStages(ColorSpace cs,
                          pipeline::RasterPipelineBuilder& p) const {
  using Stage = pipeline::Stage;

  const auto ts = transform_.invert();
  if (!ts.has_value()) return false;

  p.push(Stage::SeedShader);
  p.pushTransform(*ts);

  auto quality = quality_;

  // Reduce quality if transform is identity or translate-only.
  if (ts->isIdentity() || ts->isTranslate()) {
    quality = FilterQuality::Nearest;
  }

  if (quality == FilterQuality::Bilinear && ts->isTranslate()) {
    if (ts->tx == std::trunc(ts->tx) && ts->ty == std::trunc(ts->ty)) {
      quality = FilterQuality::Nearest;
    }
  }

  switch (quality) {
    case FilterQuality::Nearest: {
      p.ctx().limit_x = pipeline::TileCtx{
          .scale = static_cast<float>(pixmap_.width()),
          .inv_scale = 1.0f / static_cast<float>(pixmap_.width())};
      p.ctx().limit_y = pipeline::TileCtx{
          .scale = static_cast<float>(pixmap_.height()),
          .inv_scale = 1.0f / static_cast<float>(pixmap_.height())};

      switch (spread_mode_) {
        case SpreadMode::Pad:
          break;  // Gather stage will clamp.
        case SpreadMode::Repeat:
          p.push(Stage::Repeat);
          break;
        case SpreadMode::Reflect:
          p.push(Stage::Reflect);
          break;
      }

      p.push(Stage::Gather);
      break;
    }
    case FilterQuality::Bilinear: {
      p.ctx().sampler = pipeline::SamplerCtx{
          .spread_mode = spread_mode_,
          .inv_width = 1.0f / static_cast<float>(pixmap_.width()),
          .inv_height = 1.0f / static_cast<float>(pixmap_.height())};
      p.push(Stage::Bilinear);
      break;
    }
    case FilterQuality::Bicubic: {
      p.ctx().sampler = pipeline::SamplerCtx{
          .spread_mode = spread_mode_,
          .inv_width = 1.0f / static_cast<float>(pixmap_.width()),
          .inv_height = 1.0f / static_cast<float>(pixmap_.height())};
      p.push(Stage::Bicubic);
      p.push(Stage::Clamp0);
      p.push(Stage::ClampA);
      break;
    }
  }

  if (opacity_ != NormalizedF32::one()) {
    p.ctx().current_coverage = opacity_.get();
    p.push(Stage::Scale1Float);
  }

  const auto expandSt = expandStage(cs);
  if (expandSt.has_value()) {
    p.push(*expandSt);
  }

  return true;
}

// ---------------------------------------------------------------------------
// Shader free functions
// ---------------------------------------------------------------------------

bool isShaderOpaque(const Shader& shader) {
  return std::visit(
      [](const auto& s) -> bool {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, Color>) {
          return s.isOpaque();
        } else if constexpr (std::is_same_v<T, LinearGradient> ||
                             std::is_same_v<T, SweepGradient> ||
                             std::is_same_v<T, RadialGradient>) {
          return s.isOpaque();
        } else if constexpr (std::is_same_v<T, Pattern>) {
          return s.isOpaque();
        }
        return false;
      },
      shader);
}

bool pushShaderStages(const Shader& shader, ColorSpace cs,
                      pipeline::RasterPipelineBuilder& p) {
  return std::visit(
      [&](const auto& s) -> bool {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, Color>) {
          const auto premult = expandColor(cs, s).premultiply();
          p.pushUniformColor(premult);
          return true;
        } else if constexpr (std::is_same_v<T, LinearGradient> ||
                             std::is_same_v<T, SweepGradient> ||
                             std::is_same_v<T, RadialGradient>) {
          return s.pushStages(cs, p);
        } else if constexpr (std::is_same_v<T, Pattern>) {
          return s.pushStages(cs, p);
        }
        return false;
      },
      shader);
}

void transformShader(Shader& shader, const Transform& ts) {
  std::visit(
      [&](auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, Color>) {
          // SolidColor is transform-invariant.
        } else if constexpr (std::is_same_v<T, LinearGradient> ||
                             std::is_same_v<T, SweepGradient> ||
                             std::is_same_v<T, RadialGradient>) {
          s.base_.transform = s.base_.transform.postConcat(ts);
        } else if constexpr (std::is_same_v<T, Pattern>) {
          s.transform_ = s.transform_.postConcat(ts);
        }
      },
      shader);
}

void applyShaderOpacity(Shader& shader, float opacity) {
  std::visit(
      [&](auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, Color>) {
          s.applyOpacity(opacity);
        } else if constexpr (std::is_same_v<T, LinearGradient> ||
                             std::is_same_v<T, SweepGradient> ||
                             std::is_same_v<T, RadialGradient>) {
          s.base_.applyOpacity(opacity);
        } else if constexpr (std::is_same_v<T, Pattern>) {
          s.opacity_ = NormalizedF32::newClamped(s.opacity_.get() * bound(0.0f, opacity, 1.0f));
        }
      },
      shader);
}

}  // namespace tiny_skia
