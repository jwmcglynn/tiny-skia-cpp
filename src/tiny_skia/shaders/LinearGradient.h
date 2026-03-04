#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "tiny_skia/shaders/Gradient.h"

namespace tiny_skia {

// A linear gradient shader. Matches Rust `LinearGradient`.
class LinearGradient {
 public:
  // Creates a new linear gradient. Returns Shader or nullopt.
  static std::optional<std::variant<Color, LinearGradient>> create(Point start, Point end,
                                                                   std::vector<GradientStop> stops,
                                                                   SpreadMode mode,
                                                                   Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs, pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;
};

}  // namespace tiny_skia
