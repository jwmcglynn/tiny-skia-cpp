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
// Shader free functions
// ---------------------------------------------------------------------------

bool isShaderOpaque(const Shader& shader) {
  return std::visit(
      [](const auto& s) -> bool {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, Color>) {
          return s.isOpaque();
        } else if constexpr (std::is_same_v<T, LinearGradient>) {
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
        } else if constexpr (std::is_same_v<T, LinearGradient>) {
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
        } else if constexpr (std::is_same_v<T, LinearGradient>) {
          s.base_.transform = s.base_.transform.postConcat(ts);
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
        } else if constexpr (std::is_same_v<T, LinearGradient>) {
          s.base_.applyOpacity(opacity);
        }
      },
      shader);
}

}  // namespace tiny_skia
