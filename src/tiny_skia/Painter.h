#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Mask.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/Stroke.h"
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

/// Splits the target pixmap into a list of tiles for large pixmaps.
class DrawTiler {
 public:
  static constexpr std::uint32_t kMaxDimensions = 8192 - 1;

  /// Returns true if tiling is required for the given dimensions.
  [[nodiscard]] static bool required(std::uint32_t imageWidth, std::uint32_t imageHeight) {
    return imageWidth > kMaxDimensions || imageHeight > kMaxDimensions;
  }

  /// Creates a tiler if tiling is required, otherwise returns nullopt.
  [[nodiscard]] static std::optional<DrawTiler> create(std::uint32_t imageWidth,
                                                       std::uint32_t imageHeight) {
    if (required(imageWidth, imageHeight)) {
      return DrawTiler(imageWidth, imageHeight);
    }
    return std::nullopt;
  }

  /// Returns the next tile, or nullopt when iteration is complete.
  [[nodiscard]] std::optional<ScreenIntRect> next() {
    if (finished_) {
      return std::nullopt;
    }

    if (x_offset_ < image_width_ && y_offset_ < image_height_) {
      const auto h = (y_offset_ < image_height_)
                         ? std::min(image_height_ - y_offset_, kMaxDimensions)
                         : image_height_;

      const auto r = ScreenIntRect::fromXYWH(x_offset_, y_offset_,
                                             std::min(image_width_ - x_offset_, kMaxDimensions), h);

      x_offset_ += kMaxDimensions;
      if (x_offset_ >= image_width_) {
        x_offset_ = 0;
        y_offset_ += kMaxDimensions;
      }

      return r;
    }

    return std::nullopt;
  }

 private:
  explicit DrawTiler(std::uint32_t imageWidth, std::uint32_t imageHeight)
      : image_width_(imageWidth), image_height_(imageHeight) {}

  std::uint32_t image_width_ = 0;
  std::uint32_t image_height_ = 0;
  std::uint32_t x_offset_ = 0;
  std::uint32_t y_offset_ = 0;
  bool finished_ = false;
};

/// Returns true if the path's bounds are too large for fixed-point math.
[[nodiscard]] bool isTooBigForMath(const Path& path);

/// Determines if a stroke should be treated as a hairline.
/// Returns the coverage scaling factor, or nullopt for thick strokes.
[[nodiscard]] std::optional<float> treatAsHairline(const Paint& paint, float strokeWidth,
                                                   Transform ts);

// ---- Drawing functions (on PixmapMut) ----

/// Draws a filled rectangle onto the pixmap.
void fillRect(PixmapMut& pixmap, const Rect& rect, const Paint& paint, Transform transform,
              const Mask* mask = nullptr);

/// Draws a filled path onto the pixmap.
void fillPath(PixmapMut& pixmap, const Path& path, const Paint& paint, FillRule fillRule,
              Transform transform, const Mask* mask = nullptr);

/// Draws a pixmap on top of the current pixmap.
void drawPixmap(PixmapMut& pixmap, std::int32_t x, std::int32_t y, PixmapRef src,
                const PixmapPaint& paint, Transform transform, const Mask* mask = nullptr);

/// Applies a mask to already-drawn content.
void applyMask(PixmapMut& pixmap, const Mask& mask);

/// Strokes a path onto the pixmap.
void strokePath(PixmapMut& pixmap, const Path& path, const Paint& paint, const Stroke& stroke,
                Transform transform, const Mask* mask = nullptr);

/// Strokes a path with a hairline (subpixel width).
void strokeHairline(const Path& path, const Paint& paint, LineCap lineCap,
                    std::optional<SubMaskRef> mask, SubPixmapMut& subpix);

}  // namespace tiny_skia
