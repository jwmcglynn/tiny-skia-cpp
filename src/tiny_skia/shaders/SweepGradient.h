#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "tiny_skia/shaders/Gradient.h"

namespace tiny_skia {

// A sweep gradient shader.
class SweepGradient {
 public:
  explicit SweepGradient(Gradient base) : base_(std::move(base)) {}

  // Creates a new sweep gradient. Returns Shader or nullopt.
  static std::optional<std::variant<Color, SweepGradient>> create(Point center, float startAngle,
                                                                  float endAngle,
                                                                  std::vector<GradientStop> stops,
                                                                  SpreadMode mode,
                                                                  Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs, pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;

 private:
  float t0_ = 0.0f;
  float t1_ = 1.0f;
};

}  // namespace tiny_skia
