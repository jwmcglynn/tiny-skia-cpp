#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

#include "tiny_skia/Color.h"
#include "tiny_skia/Point.h"
#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia {

// Forward declarations for gradient types defined in their own headers.
class LinearGradient;
class RadialGradient;
class SweepGradient;
class Pattern;

/// The default SCALAR_NEARLY_ZERO threshold is too big for gradients.
/// Matches Rust `gradient::DEGENERATE_THRESHOLD`.
constexpr float kDegenerateThreshold = 1.0f / (1 << 15);

/// A gradient stop.  Matches Rust `GradientStop`.
struct GradientStop {
  NormalizedF32 position;
  Color color;

  static GradientStop create(float position, Color color) {
    return GradientStop{NormalizedF32::newClamped(position), color};
  }
};

/// Base gradient data.  Matches Rust `Gradient`.
class Gradient {
 public:
  Gradient(std::vector<GradientStop> stops, SpreadMode tileMode,
           Transform transform, Transform pointsToUnit);

  [[nodiscard]] bool colorsAreOpaque() const { return colors_are_opaque_; }

  [[nodiscard]] bool pushStages(
      pipeline::RasterPipelineBuilder& p, ColorSpace cs,
      const std::function<void(pipeline::RasterPipelineBuilder&)>& pushStagesPre,
      const std::function<void(pipeline::RasterPipelineBuilder&)>& pushStagesPost) const;

  void applyOpacity(float opacity);

  Transform transform;

 private:
  std::vector<GradientStop> stops_;
  SpreadMode tile_mode_ = SpreadMode::Pad;
  Transform points_to_unit_;
  bool colors_are_opaque_ = true;
  bool has_uniform_stops_ = true;
};

/// A linear gradient shader.  Matches Rust `LinearGradient`.
class LinearGradient {
 public:
  /// Creates a new linear gradient.  Returns Shader or nullopt.
  static std::optional<std::variant<Color, LinearGradient>> create(
      Point start, Point end, std::vector<GradientStop> stops,
      SpreadMode mode, Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;
};

/// The Shader variant type.  Matches Rust `enum Shader`.
/// Uses std::variant to dispatch between shader types.
///
/// Note: RadialGradient, SweepGradient, and Pattern are not yet implemented.
/// They are represented as placeholder types here.
using Shader = std::variant<Color, LinearGradient>;

/// Returns true if the shader is guaranteed to produce only opaque colors.
[[nodiscard]] bool isShaderOpaque(const Shader& shader);

/// Pushes shader stages into the pipeline builder.
/// Returns false if the shader fails (e.g., non-invertible transform).
[[nodiscard]] bool pushShaderStages(const Shader& shader, ColorSpace cs,
                                    pipeline::RasterPipelineBuilder& p);

/// Applies a transform to the shader.
void transformShader(Shader& shader, const Transform& ts);

/// Applies opacity to the shader.
void applyShaderOpacity(Shader& shader, float opacity);

}  // namespace tiny_skia
