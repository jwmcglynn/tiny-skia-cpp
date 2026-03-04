#pragma once

#include <functional>
#include <vector>

#include "tiny_skia/Color.h"
#include "tiny_skia/Point.h"
#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia {

// The default SCALAR_NEARLY_ZERO threshold is too big for gradients.
constexpr float kDegenerateThreshold = 1.0f / (1 << 15);

// A gradient stop.
struct GradientStop {
  NormalizedF32 position;
  Color color;

  static GradientStop create(float position, Color color) {
    return GradientStop{NormalizedF32::newClamped(position), color};
  }
};

// Base gradient data.
class Gradient {
 public:
  Gradient(std::vector<GradientStop> stops, SpreadMode tileMode, Transform transform,
           Transform pointsToUnit);

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

}  // namespace tiny_skia
