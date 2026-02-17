// Port of Mask::fill_path and Mask::intersect_path from mask.rs.
// Lives in the painter target to avoid a circular dependency between
// tiny_skia_core and tiny_skia_scan.

#include "tiny_skia/Mask.h"

#include "tiny_skia/Color.h"
#include "tiny_skia/Math.h"
#include "tiny_skia/Painter.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/pipeline/Blitter.h"
#include "tiny_skia/scan/Path.h"
#include "tiny_skia/scan/PathAa.h"

namespace tiny_skia {

void Mask::fillPath(const Path& path, FillRule fillRule, bool antiAlias,
                    Transform transform) {
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

    if (auto tiler = DrawTiler::create(width(), height())) {
      auto pathCopy = path;

      while (auto tile = tiler->next()) {
        const auto ts =
            Transform::fromTranslate(-static_cast<float>(tile->x()),
                                     -static_cast<float>(tile->y()));
        auto transformed = pathCopy.transform(ts);
        if (!transformed.has_value()) {
          return;
        }
        pathCopy = std::move(*transformed);

        const auto clipRect = tile->size().toScreenIntRect(0, 0);
        auto subpix = subpixmap(tile->toIntRect());
        if (!subpix.has_value()) {
          continue;
        }

        // Convert SubMaskMut to SubPixmapMut for the pipeline blitter.
        SubPixmapMut pix{};
        pix.size = subpix->size;
        pix.real_width = static_cast<std::size_t>(subpix->realWidth);
        pix.data = subpix->data;

        auto blitter = pipeline::RasterPipelineBlitter::createMask(&pix);
        if (!blitter.has_value()) {
          continue;
        }

        if (antiAlias) {
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
      }
    } else {
      const auto clipRect = size_.toScreenIntRect(0, 0);

      // Convert full mask to SubPixmapMut.
      SubPixmapMut pix{};
      pix.size = size_;
      pix.real_width = static_cast<std::size_t>(width());
      pix.data = data_.data();

      auto blitter = pipeline::RasterPipelineBlitter::createMask(&pix);
      if (!blitter.has_value()) {
        return;
      }

      if (antiAlias) {
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
    fillPath(*transformed, fillRule, antiAlias, Transform::identity());
  }
}

void Mask::intersectPath(const Path& path, FillRule fillRule, bool antiAlias,
                         Transform transform) {
  auto submask = Mask::fromSize(width(), height());
  if (!submask.has_value()) {
    return;
  }
  submask->fillPath(path, fillRule, antiAlias, transform);

  for (std::size_t i = 0; i < data_.size(); ++i) {
    data_[i] = premultiplyU8(data_[i], submask->data_[i]);
  }
}

}  // namespace tiny_skia
