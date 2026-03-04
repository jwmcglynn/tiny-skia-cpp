#pragma once

#include <cstdint>

#include "tiny_skia/BlendMode.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/shaders/Mod.h"

namespace tiny_skia {

/// Controls how a shape should be painted.
struct Paint {
  /// A paint shader.  Default: black color.
  Shader shader = Color::black;

  /// Paint blending mode.  Default: SourceOver.
  BlendMode blendMode = BlendMode::SourceOver;

  /// Enables anti-aliased painting.  Default: true.
  bool antiAlias = true;

  /// Colorspace for blending (gamma correction).  Default: Linear.
  ColorSpace colorspace = ColorSpace::Linear;

  /// Forces the high quality/precision rendering pipeline.  Default: false.
  bool forceHqPipeline = false;

  /// Sets a paint source to a solid color.
  void setColor(const Color& color) { shader = color; }

  /// Sets a paint source to a solid color from RGBA8 components.
  void setColorRgba8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    setColor(Color::fromRgba8(r, g, b, a));
  }

  /// Checks that the paint source is a solid color.
  [[nodiscard]] bool isSolidColor() const { return std::holds_alternative<Color>(shader); }
};

}  // namespace tiny_skia
