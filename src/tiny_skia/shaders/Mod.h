#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

#include "tiny_skia/BlendMode.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/Point.h"
#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia {

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

/// A sweep gradient shader.  Matches Rust `SweepGradient`.
class SweepGradient {
 public:
  explicit SweepGradient(Gradient base) : base_(std::move(base)) {}

  /// Creates a new sweep gradient.  Returns Shader or nullopt.
  static std::optional<std::variant<Color, SweepGradient>> create(
      Point center, float startAngle, float endAngle,
      std::vector<GradientStop> stops, SpreadMode mode, Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;

 private:
  float t0_ = 0.0f;
  float t1_ = 1.0f;
};

/// Focal point data for two-point conical gradients.
/// Matches Rust `FocalData`.
struct FocalData {
  float r1 = 0.0f;
  float focal_x = 0.0f;
  bool is_swapped = false;

  [[nodiscard]] bool isFocalOnCircle() const;
  [[nodiscard]] bool isWellBehaved() const;
  [[nodiscard]] bool isNativelyFocal() const;

  bool set(float r0, float r1, Transform& matrix);
};

/// Gradient type classification for radial gradients.
/// Matches Rust `GradientType`.
struct RadialType {
  float radius1 = 0.0f;
  float radius2 = 0.0f;
};

struct StripType {
  float scaled_r0 = 0.0f;
};

using GradientType = std::variant<RadialType, StripType, FocalData>;

/// A radial (two-point conical) gradient shader.
/// Matches Rust `RadialGradient`.
class RadialGradient {
 public:
  explicit RadialGradient(Gradient base) : base_(std::move(base)) {}

  /// Creates a new radial gradient.  Returns Shader or nullopt.
  static std::optional<std::variant<Color, RadialGradient>> create(
      Point startPoint, float startRadius, Point endPoint, float endRadius,
      std::vector<GradientStop> stops, SpreadMode mode, Transform transform);

  [[nodiscard]] bool isOpaque() const { return base_.colorsAreOpaque(); }

  [[nodiscard]] bool pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const;

  Gradient base_;

 private:
  static std::optional<std::variant<Color, RadialGradient>> createRadialUnchecked(
      Point center, float radius, std::vector<GradientStop> stops,
      SpreadMode mode, Transform transform);

  static std::optional<std::variant<Color, RadialGradient>> createTwoPoint(
      Point c0, float r0, Point c1, float r1,
      std::vector<GradientStop> stops, SpreadMode mode, Transform transform);

  GradientType gradient_type_;
};

/// Filter quality for pattern sampling.  Matches Rust `FilterQuality`.
enum class FilterQuality {
  Nearest,
  Bilinear,
  Bicubic,
};

/// Paint settings for pixmap drawing.  Matches Rust `PixmapPaint`.
struct PixmapPaint {
  float opacity = 1.0f;
  BlendMode blend_mode = BlendMode::SourceOver;
  FilterQuality quality = FilterQuality::Nearest;
};

/// A pattern shader (pixmap-based).  Matches Rust `Pattern`.
class Pattern {
 public:
  Pattern(PixmapRef pixmap, SpreadMode spreadMode, FilterQuality quality,
          float opacity, Transform transform);

  [[nodiscard]] bool isOpaque() const;

  [[nodiscard]] bool pushStages(ColorSpace cs,
                                pipeline::RasterPipelineBuilder& p) const;

  PixmapRef pixmap_;
  NormalizedF32 opacity_;
  Transform transform_;

 private:
  FilterQuality quality_;
  SpreadMode spread_mode_;
};

/// The Shader variant type.  Matches Rust `enum Shader`.
/// Uses std::variant to dispatch between shader types.
using Shader = std::variant<Color, LinearGradient, SweepGradient, RadialGradient, Pattern>;

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
