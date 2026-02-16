#include "tiny_skia/pipeline/Blitter.h"

#include <algorithm>

namespace tiny_skia::pipeline {
namespace {

constexpr std::size_t kBytesPerPixel = 4;

[[nodiscard]] std::size_t alphaOffset(const SubPixmapMut& pixmap, std::size_t x, std::size_t y) {
  return (y * pixmap.real_width + x) * kBytesPerPixel + 3;
}

[[nodiscard]] AlphaU8 lerpMaskAlpha(AlphaU8 dst, AlphaU8 coverage) {
  const auto inv = static_cast<std::uint32_t>(255u - dst);
  const auto scaled = (inv * static_cast<std::uint32_t>(coverage) + 127u) / 255u;
  return static_cast<AlphaU8>(dst + scaled);
}


[[nodiscard]] AlphaU8 externalMaskCoverage(const std::optional<SubMaskRef>& mask,
                                           std::size_t x,
                                           std::size_t y) {
  if (!mask.has_value()) {
    return 255u;
  }

  if (x >= mask->size.width() || y >= mask->size.height()) {
    return 0u;
  }

  const auto offset = y * static_cast<std::size_t>(mask->realWidth) + x;
  return mask->data[offset];
}

[[nodiscard]] AlphaU8 combineCoverage(AlphaU8 coverage, AlphaU8 maskCoverage) {
  const auto scaled =
      static_cast<std::uint32_t>(coverage) * static_cast<std::uint32_t>(maskCoverage);
  return static_cast<AlphaU8>((scaled + 127u) / 255u);
}

}  // namespace

RasterPipelineBlitter::RasterPipelineBlitter(SubPixmapMut* pixmap,
                                             bool isMaskOnly,
                                             PremultipliedColorU8 memsetColor,
                                             std::optional<SubMaskRef> mask)
    : pixmap_(pixmap), is_mask_only_(isMaskOnly), memset_color_(memsetColor), mask_(mask) {}

std::optional<RasterPipelineBlitter> RasterPipelineBlitter::create(
    PremultipliedColorU8 color,
    SubPixmapMut* pixmap,
    std::optional<SubMaskRef> mask) {
  if (pixmap == nullptr) {
    return std::nullopt;
  }

  if (mask.has_value()) {
    if (mask->size.width() != pixmap->width() || mask->size.height() != pixmap->height()) {
      return std::nullopt;
    }
  }

  return RasterPipelineBlitter(pixmap, false, color, mask);
}

std::optional<RasterPipelineBlitter> RasterPipelineBlitter::createMask(SubPixmapMut* pixmap) {
  if (pixmap == nullptr) {
    return std::nullopt;
  }

  return RasterPipelineBlitter(pixmap, true, Color::white.premultiply().toColorU8(), std::nullopt);
}

void RasterPipelineBlitter::blitH(std::uint32_t x, std::uint32_t y, LengthU32 width) {
  blitRect(ScreenIntRect::fromXYWHSafe(x, y, width, 1u));
}

void RasterPipelineBlitter::blitAntiH(std::uint32_t x,
                                      std::uint32_t y,
                                      std::span<std::uint8_t> alpha,
                                      std::span<AlphaRun> runs) {
  if (pixmap_ == nullptr || alpha.empty() || runs.empty()) {
    return;
  }

  std::size_t aa_offset = 0;
  std::size_t run_offset = 0;
  while (run_offset < runs.size() && runs[run_offset].has_value()) {
    const auto run = static_cast<LengthU32>(runs[run_offset].value());
    if (run == 0u || aa_offset >= alpha.size()) {
      break;
    }

    const auto coverage = alpha[aa_offset];
    if (coverage == 255u) {
      blitH(x, y, run);
    } else if (coverage != 0u) {
      blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x, y, run, 1u), coverage);
    }

    x += run;
    run_offset += static_cast<std::size_t>(run);
    aa_offset += static_cast<std::size_t>(run);
  }
}

void RasterPipelineBlitter::blitV(std::uint32_t x,
                                  std::uint32_t y,
                                  LengthU32 height,
                                  AlphaU8 alpha) {
  if (alpha == 0u) {
    return;
  }

  blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x, y, 1u, height), alpha);
}

void RasterPipelineBlitter::blitAntiH2(std::uint32_t x,
                                       std::uint32_t y,
                                       AlphaU8 alpha0,
                                       AlphaU8 alpha1) {
  if (alpha0 != 0u) {
    blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x, y, 1u, 1u), alpha0);
  }
  if (alpha1 != 0u) {
    blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x + 1u, y, 1u, 1u), alpha1);
  }
}

void RasterPipelineBlitter::blitAntiV2(std::uint32_t x,
                                       std::uint32_t y,
                                       AlphaU8 alpha0,
                                       AlphaU8 alpha1) {
  if (alpha0 != 0u) {
    blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x, y, 1u, 1u), alpha0);
  }
  if (alpha1 != 0u) {
    blendMaskRectConstant(ScreenIntRect::fromXYWHSafe(x, y + 1u, 1u, 1u), alpha1);
  }
}

void RasterPipelineBlitter::blitRect(const ScreenIntRect& rect) {
  if (pixmap_ == nullptr) {
    return;
  }

  if (is_mask_only_) {
    fillMaskRect(rect, memset_color_.alpha());
    return;
  }

  auto bytes = pixmap_->dataMut();
  const auto maxX = std::min<std::size_t>(pixmap_->width(), rect.x() + rect.width());
  const auto maxY = std::min<std::size_t>(pixmap_->height(), rect.y() + rect.height());

  for (std::size_t yy = rect.y(); yy < maxY; ++yy) {
    for (std::size_t xx = rect.x(); xx < maxX; ++xx) {
      const auto offset = (yy * pixmap_->real_width + xx) * kBytesPerPixel;
      const auto maskCoverage = externalMaskCoverage(mask_, xx, yy);
      if (maskCoverage == 0u) {
        continue;
      }

      bytes[offset + 0] = memset_color_.red();
      bytes[offset + 1] = memset_color_.green();
      bytes[offset + 2] = memset_color_.blue();
      bytes[offset + 3] = combineCoverage(memset_color_.alpha(), maskCoverage);
    }
  }
}

void RasterPipelineBlitter::blitMask(const Mask& mask, const ScreenIntRect& clip) {
  if (pixmap_ == nullptr) {
    return;
  }

  const auto maskData = mask.data();
  const auto maxWidth = std::min<std::size_t>(clip.width(), mask.width());
  const auto maxHeight = std::min<std::size_t>(clip.height(), mask.height());
  const auto maxX = std::min<std::size_t>(pixmap_->width(), clip.x() + maxWidth);
  const auto maxY = std::min<std::size_t>(pixmap_->height(), clip.y() + maxHeight);
  auto bytes = pixmap_->dataMut();

  for (std::size_t yy = clip.y(); yy < maxY; ++yy) {
    const auto maskY = yy - clip.y();
    for (std::size_t xx = clip.x(); xx < maxX; ++xx) {
      const auto maskX = xx - clip.x();
      const auto coverage = maskData[maskY * mask.width() + maskX];
      if (coverage == 0u) {
        continue;
      }

      const auto externalCoverage = externalMaskCoverage(mask_, xx, yy);
      const auto combinedCoverage = combineCoverage(coverage, externalCoverage);
      if (combinedCoverage == 0u) {
        continue;
      }

      const auto offset = alphaOffset(*pixmap_, xx, yy);
      if (is_mask_only_) {
        bytes[offset] = lerpMaskAlpha(bytes[offset], combinedCoverage);
        continue;
      }

      const auto pixelOffset = (yy * pixmap_->real_width + xx) * kBytesPerPixel;
      bytes[pixelOffset + 0] = memset_color_.red();
      bytes[pixelOffset + 1] = memset_color_.green();
      bytes[pixelOffset + 2] = memset_color_.blue();
      const auto srcAlpha = combineCoverage(memset_color_.alpha(), combinedCoverage);
      bytes[offset] = lerpMaskAlpha(bytes[offset], srcAlpha);
    }
  }
}

void RasterPipelineBlitter::fillMaskRect(const ScreenIntRect& rect, AlphaU8 alpha) {
  auto bytes = pixmap_->dataMut();
  const auto maxX = std::min<std::size_t>(pixmap_->width(), rect.x() + rect.width());
  const auto maxY = std::min<std::size_t>(pixmap_->height(), rect.y() + rect.height());

  for (std::size_t yy = rect.y(); yy < maxY; ++yy) {
    for (std::size_t xx = rect.x(); xx < maxX; ++xx) {
      const auto maskCoverage = externalMaskCoverage(mask_, xx, yy);
      const auto coverage = combineCoverage(alpha, maskCoverage);
      bytes[alphaOffset(*pixmap_, xx, yy)] = coverage;
    }
  }
}

void RasterPipelineBlitter::blendMaskRectConstant(const ScreenIntRect& rect, AlphaU8 alpha) {
  if (pixmap_ == nullptr || alpha == 0u) {
    return;
  }

  auto bytes = pixmap_->dataMut();
  const auto maxX = std::min<std::size_t>(pixmap_->width(), rect.x() + rect.width());
  const auto maxY = std::min<std::size_t>(pixmap_->height(), rect.y() + rect.height());

  for (std::size_t yy = rect.y(); yy < maxY; ++yy) {
    for (std::size_t xx = rect.x(); xx < maxX; ++xx) {
      const auto maskCoverage = externalMaskCoverage(mask_, xx, yy);
      const auto coverage = combineCoverage(alpha, maskCoverage);
      if (coverage == 0u) {
        continue;
      }

      const auto offset = alphaOffset(*pixmap_, xx, yy);
      if (is_mask_only_) {
        bytes[offset] = lerpMaskAlpha(bytes[offset], coverage);
        continue;
      }

      const auto pixelOffset = (yy * pixmap_->real_width + xx) * kBytesPerPixel;
      bytes[pixelOffset + 0] = memset_color_.red();
      bytes[pixelOffset + 1] = memset_color_.green();
      bytes[pixelOffset + 2] = memset_color_.blue();
      const auto srcAlpha = combineCoverage(memset_color_.alpha(), coverage);
      bytes[offset] = lerpMaskAlpha(bytes[offset], srcAlpha);
    }
  }
}

}  // namespace tiny_skia::pipeline
