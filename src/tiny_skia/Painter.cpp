#include "tiny_skia/Painter.h"

#include <cmath>
#include <limits>

#include "tiny_skia/Math.h"
#include "tiny_skia/pipeline/Blitter.h"
#include "tiny_skia/scan/Hairline.h"
#include "tiny_skia/scan/HairlineAa.h"
#include "tiny_skia/scan/Mod.h"
#include "tiny_skia/scan/Path.h"
#include "tiny_skia/scan/PathAa.h"

namespace tiny_skia {

namespace {

/// Matches Rust `SCALAR_MAX` from tiny-skia-path.
constexpr float kScalarMax = std::numeric_limits<float>::max();

}  // namespace

bool isTooBigForMath(const Path& path) {
  constexpr float kScaleDownToAllowForSmallMultiplies = 0.25f;
  constexpr float kMax = kScalarMax * kScaleDownToAllowForSmallMultiplies;

  const auto b = path.bounds();

  // Use ! expression so we return true if bounds contains NaN.
  return !(b.left() >= -kMax && b.top() >= -kMax && b.right() <= kMax &&
           b.bottom() <= kMax);
}

std::optional<float> treatAsHairline(const Paint& paint, float strokeWidth,
                                     Transform ts) {
  if (strokeWidth == 0.0f) {
    return 1.0f;
  }

  if (!paint.anti_alias) {
    return std::nullopt;
  }

  // We don't care about translate.
  ts.tx = 0.0f;
  ts.ty = 0.0f;

  // Map the stroke width through the transform to check actual pixel width.
  Point points[2] = {Point::fromXy(strokeWidth, 0.0f),
                     Point::fromXy(0.0f, strokeWidth)};
  ts.mapPoints(points);

  // fast_len: approximation of vector length.
  auto fastLen = [](Point p) -> float {
    float ax = std::abs(p.x);
    float ay = std::abs(p.y);
    if (ax < ay) {
      std::swap(ax, ay);
    }
    return ax + ay * 0.5f;
  };

  const float len0 = fastLen(points[0]);
  const float len1 = fastLen(points[1]);

  if (len0 <= 1.0f && len1 <= 1.0f) {
    return (len0 + len1) * 0.5f;
  }

  return std::nullopt;
}

void fillRect(PixmapMut& pixmap, const Rect& rect, const Paint& paint,
              Transform transform, const Mask* mask) {
  if (transform.isIdentity() &&
      !DrawTiler::required(pixmap.width(), pixmap.height())) {
    const auto clip = pixmap.size().toScreenIntRect(0, 0);

    auto submaskOpt =
        mask ? std::optional<SubMaskRef>(mask->asSubmask()) : std::nullopt;
    auto subpix = pixmap.asSubpixmap();
    auto blitter =
        pipeline::RasterPipelineBlitter::create(paint, submaskOpt, &subpix);
    if (!blitter.has_value()) {
      return;
    }

    if (paint.anti_alias) {
      scan::fillRectAa(rect, clip, *blitter);
    } else {
      scan::fillRect(rect, clip, *blitter);
    }
  } else {
    const auto path = pathFromRect(rect);
    fillPath(pixmap, path, paint, FillRule::Winding, transform, mask);
  }
}

void fillPath(PixmapMut& pixmap, const Path& path, const Paint& paint,
              FillRule fillRule, Transform transform, const Mask* mask) {
  if (transform.isIdentity()) {
    // Skip empty paths and horizontal/vertical lines.
    const auto pathBounds = path.bounds();
    if (isNearlyZero(pathBounds.width()) ||
        isNearlyZero(pathBounds.height())) {
      return;
    }

    if (isTooBigForMath(path)) {
      return;
    }

    if (auto tiler = DrawTiler::create(pixmap.width(), pixmap.height())) {
      auto pathCopy = path;
      auto paintCopy = paint;

      while (auto tile = tiler->next()) {
        const auto ts =
            Transform::fromTranslate(-static_cast<float>(tile->x()),
                                     -static_cast<float>(tile->y()));
        auto transformed = pathCopy.transform(ts);
        if (!transformed.has_value()) {
          return;
        }
        pathCopy = std::move(*transformed);
        transformShader(paintCopy.shader, ts);

        const auto clipRect = tile->size().toScreenIntRect(0, 0);
        auto subpix = pixmap.subpixmap(tile->toIntRect());
        if (!subpix.has_value()) {
          continue;
        }

        auto submaskOpt =
            mask ? mask->submask(tile->toIntRect()) : std::nullopt;
        auto blitter = pipeline::RasterPipelineBlitter::create(
            paintCopy, submaskOpt, &(*subpix));
        if (!blitter.has_value()) {
          continue;
        }

        if (paintCopy.anti_alias) {
          scan::path_aa::fillPath(pathCopy, fillRule, clipRect, *blitter);
        } else {
          scan::fillPath(pathCopy, fillRule, clipRect, *blitter);
        }

        const auto tsBack =
            Transform::fromTranslate(static_cast<float>(tile->x()),
                                     static_cast<float>(tile->y()));
        auto untransformed = pathCopy.transform(tsBack);
        if (!untransformed.has_value()) {
          return;
        }
        pathCopy = std::move(*untransformed);
        transformShader(paintCopy.shader, tsBack);
      }
    } else {
      const auto clipRect = pixmap.size().toScreenIntRect(0, 0);
      auto submaskOpt =
          mask ? std::optional<SubMaskRef>(mask->asSubmask()) : std::nullopt;
      auto subpix = pixmap.asSubpixmap();
      auto blitter =
          pipeline::RasterPipelineBlitter::create(paint, submaskOpt, &subpix);
      if (!blitter.has_value()) {
        return;
      }

      if (paint.anti_alias) {
        scan::path_aa::fillPath(path, fillRule, clipRect, *blitter);
      } else {
        scan::fillPath(path, fillRule, clipRect, *blitter);
      }
    }
  } else {
    auto transformed = path.transform(transform);
    if (!transformed.has_value()) {
      return;
    }

    auto paintCopy = paint;
    transformShader(paintCopy.shader, transform);

    fillPath(pixmap, *transformed, paintCopy, fillRule, Transform::identity(),
             mask);
  }
}

void drawPixmap(PixmapMut& pixmap, std::int32_t x, std::int32_t y,
                PixmapRef src, const PixmapPaint& ppaint, Transform transform,
                const Mask* mask) {
  const auto intRect = src.size().toIntRect(x, y);
  if (!intRect.has_value()) {
    return;
  }
  const auto screenRect = intRect->toScreenIntRect();
  if (!screenRect.has_value()) {
    return;
  }
  const auto rect = screenRect->toRect();

  // Translate pattern as well as bounds.
  const auto pattTransform =
      Transform::fromTranslate(static_cast<float>(x), static_cast<float>(y));

  Paint paint;
  paint.shader = Pattern(src, SpreadMode::Pad, ppaint.quality, ppaint.opacity,
                         pattTransform);
  paint.blend_mode = ppaint.blend_mode;
  paint.anti_alias = false;
  paint.force_hq_pipeline = false;
  paint.colorspace = ColorSpace::Linear;

  fillRect(pixmap, rect, paint, transform, mask);
}

void applyMask(PixmapMut& pixmap, const Mask& mask) {
  if (pixmap.size() != mask.size()) {
    return;
  }

  // Dummy source pixmap.
  auto pixmapSrc = PixmapRef::fromBytes(
      std::span<const std::uint8_t>({0, 0, 0, 0}), 1, 1);
  if (!pixmapSrc.has_value()) {
    return;
  }

  pipeline::RasterPipelineBuilder p;
  p.push(pipeline::Stage::LoadMaskU8);
  p.push(pipeline::Stage::LoadDestination);
  p.push(pipeline::Stage::DestinationIn);
  p.push(pipeline::Stage::Store);
  auto rp = p.compile();

  const auto rect = pixmap.size().toScreenIntRect(0, 0);
  auto subpix = pixmap.asSubpixmap();
  const auto submask = mask.asSubmask();
  const auto maskCtx =
      pipeline::MaskCtx{submask.data, submask.realWidth};
  rp.run(rect, pipeline::AAMaskCtx{}, maskCtx, *pixmapSrc, &subpix);
}

void strokeHairline(const Path& path, const Paint& paint, LineCap lineCap,
                    std::optional<SubMaskRef> mask, SubPixmapMut& subpix) {
  const auto clip = subpix.size.toScreenIntRect(0, 0);
  auto blitter =
      pipeline::RasterPipelineBlitter::create(paint, mask, &subpix);
  if (!blitter.has_value()) {
    return;
  }

  if (paint.anti_alias) {
    scan::hairline_aa::strokePath(path, lineCap, clip, *blitter);
  } else {
    scan::strokePath(path, lineCap, clip, *blitter);
  }
}

}  // namespace tiny_skia
