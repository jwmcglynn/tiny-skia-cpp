#pragma once

#include <optional>

#include "tiny_skia/Color.h"

namespace tiny_skia {

enum class BlendMode {
  Clear,
  Source,
  Destination,
  SourceOver,
  DestinationOver,
  SourceIn,
  DestinationIn,
  SourceOut,
  DestinationOut,
  SourceAtop,
  DestinationAtop,
  Xor,
  Plus,
  Modulate,
  Screen,
  Overlay,
  Darken,
  Lighten,
  ColorDodge,
  ColorBurn,
  HardLight,
  SoftLight,
  Difference,
  Exclusion,
  Multiply,
  Hue,
  Saturation,
  Color,
  Luminosity,
};

[[nodiscard]] bool shouldPreScaleCoverage(BlendMode blendMode);
[[nodiscard]] std::optional<pipeline::Stage> toStage(BlendMode blendMode);

}  // namespace tiny_skia
