#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Mask.h"
#include "tiny_skia/Paint.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/Stroke.h"

namespace tiny_skia {

namespace detail {

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

    if (xOffset_ < imageWidth_ && yOffset_ < imageHeight_) {
      const auto h = (yOffset_ < imageHeight_)
                         ? std::min(imageHeight_ - yOffset_, kMaxDimensions)
                         : imageHeight_;

      const auto r = ScreenIntRect::fromXYWH(xOffset_, yOffset_,
                                             std::min(imageWidth_ - xOffset_, kMaxDimensions), h);

      xOffset_ += kMaxDimensions;
      if (xOffset_ >= imageWidth_) {
        xOffset_ = 0;
        yOffset_ += kMaxDimensions;
      }

      return r;
    }

    return std::nullopt;
  }

 private:
  explicit DrawTiler(std::uint32_t imageWidth, std::uint32_t imageHeight)
      : imageWidth_(imageWidth), imageHeight_(imageHeight) {}

  std::uint32_t imageWidth_ = 0;
  std::uint32_t imageHeight_ = 0;
  std::uint32_t xOffset_ = 0;
  std::uint32_t yOffset_ = 0;
  bool finished_ = false;
};

/// Returns true if the path's bounds are too large for fixed-point math.
[[nodiscard]] bool isTooBigForMath(const Path& path);

/// Determines if a stroke should be treated as a hairline.
/// Returns the coverage scaling factor, or nullopt for thick strokes.
[[nodiscard]] std::optional<float> treatAsHairline(const Paint& paint, float strokeWidth,
                                                   Transform ts);

}  // namespace detail

/// Groups all drawing operations. All methods are static — Painter has no
/// state and cannot be instantiated.
class Painter {
 public:
  Painter() = delete;

  /// Fills an axis-aligned rectangle onto the pixmap.
  static void fillRect(MutablePixmapView& pixmap, const Rect& rect, const Paint& paint,
                       Transform transform = Transform::identity(), const Mask* mask = nullptr);

  /// Fills a path onto the pixmap.
  static void fillPath(MutablePixmapView& pixmap, const Path& path, const Paint& paint,
                       FillRule fillRule, Transform transform = Transform::identity(),
                       const Mask* mask = nullptr);

  /// Composites a source pixmap onto a destination pixmap.
  static void drawPixmap(MutablePixmapView& pixmap, std::int32_t x, std::int32_t y, PixmapView src,
                         const PixmapPaint& paint = {}, Transform transform = Transform::identity(),
                         const Mask* mask = nullptr);

  /// Applies a mask to already-drawn content.
  static void applyMask(MutablePixmapView& pixmap, const Mask& mask);

  /// Strokes a path onto the pixmap.
  static void strokePath(MutablePixmapView& pixmap, const Path& path, const Paint& paint,
                         const Stroke& stroke, Transform transform = Transform::identity(),
                         const Mask* mask = nullptr);

 private:
  /// Strokes a path with a hairline (subpixel width).
  static void strokeHairline(const Path& path, const Paint& paint, LineCap lineCap,
                             std::optional<SubMaskView> mask, MutableSubPixmapView& subpix);
};

}  // namespace tiny_skia
