#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "tiny_skia/shaders/Gradient.h"

namespace tiny_skia {

// Focal point data for two-point conical gradients.
struct FocalData {
  float r1 = 0.0f;
  float focalX = 0.0f;
  bool isSwapped = false;

  [[nodiscard]] bool isFocalOnCircle() const;
  [[nodiscard]] bool isWellBehaved() const;
  [[nodiscard]] bool isNativelyFocal() const;

  bool set(float r0, float r1, Transform& matrix);
};

// Gradient type classification for radial gradients.
struct RadialType {
  float radius1 = 0.0f;
  float radius2 = 0.0f;
};

struct StripType {
  float scaled_r0 = 0.0f;
};

using GradientType = std::variant<RadialType, StripType, FocalData>;

// A radial (two-point conical) gradient shader.
class RadialGradient {
 public:
  explicit RadialGradient(Gradient base) : base_(std::move(base)) {}

  // Creates a new radial gradient. Returns Shader or nullopt.
  static std::optional<std::variant<Color, RadialGradient>> create(
      Point startPoint, float startRadius, Point endPoint, float endRadius,
      std::vector<GradientStop> stops, SpreadMode mode, Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs, pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;

 private:
  static std::optional<std::variant<Color, RadialGradient>> createRadialUnchecked(
      Point center, float radius, std::vector<GradientStop> stops, SpreadMode mode,
      Transform transform);

  static std::optional<std::variant<Color, RadialGradient>> createTwoPoint(
      Point c0, float r0, Point c1, float r1, std::vector<GradientStop> stops, SpreadMode mode,
      Transform transform);

  GradientType gradient_type_;
};

}  // namespace tiny_skia
