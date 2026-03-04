#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "tiny_skia/Path.h"
#include "tiny_skia/Scalar.h"

namespace tiny_skia {

/// Line join for stroked paths.
enum class LineJoin : std::uint8_t {
  Miter,
  MiterClip,
  Round,
  Bevel,
};

/// Dash pattern for stroked paths.
struct StrokeDash {
  /// Dash array. Must have an even number of entries > 0.
  std::vector<float> array;
  /// Offset into the dash pattern.
  float offset = 0.0f;

  [[nodiscard]] static std::optional<StrokeDash> create(std::vector<float> dashArray,
                                                        float dashOffset);
};

/// Stroke properties.
struct Stroke {
  float width = 1.0f;
  float miterLimit = 4.0f;
  LineCap lineCap = LineCap::Butt;
  LineJoin lineJoin = LineJoin::Miter;
  std::optional<StrokeDash> dash;
};

}  // namespace tiny_skia
