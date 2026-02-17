#include "tiny_skia/pipeline/Blitter.h"

#include <algorithm>

namespace tiny_skia::pipeline {
namespace {

constexpr std::size_t kBytesPerPixel = 4;

[[nodiscard]] MaskCtx makeMaskCtx(const std::optional<SubMaskRef>& mask) {
  if (!mask.has_value()) {
    return MaskCtx{};
  }
  return MaskCtx{mask->data, mask->realWidth};
}

[[nodiscard]] Pixmap makeDummyPixmapSrc() {
  auto p = Pixmap::fromSize(1, 1);
  return p.has_value() ? std::move(*p) : Pixmap{};
}

}  // namespace

RasterPipelineBlitter::RasterPipelineBlitter(SubPixmapMut* pixmap,
                                             bool isMaskOnly,
                                             std::optional<PremultipliedColorU8> memsetColor,
                                             std::optional<SubMaskRef> mask,
                                             Pixmap pixmapSrcStorage,
                                             RasterPipeline blitAntiHRp,
                                             RasterPipeline blitRectRp,
                                             RasterPipeline blitMaskRp)
    : pixmap_(pixmap),
      is_mask_only_(isMaskOnly),
      memset_color_(memsetColor),
      mask_(mask),
      pixmap_src_storage_(std::move(pixmapSrcStorage)),
      blit_anti_h_rp_(blitAntiHRp),
      blit_rect_rp_(blitRectRp),
      blit_mask_rp_(blitMaskRp) {}

std::optional<RasterPipelineBlitter> RasterPipelineBlitter::create(
    PremultipliedColorU8 color,
    SubPixmapMut* pixmap,
    BlendMode blendMode,
    std::optional<SubMaskRef> mask) {
  if (pixmap == nullptr) {
    return std::nullopt;
  }

  if (mask.has_value()) {
    if (mask->size.width() != pixmap->width() || mask->size.height() != pixmap->height()) {
      return std::nullopt;
    }
  }

  // Fast-reject: Destination keeps the pixmap unchanged.
  if (blendMode == BlendMode::Destination) {
    return std::nullopt;
  }

  // Strength-reduce SourceOver to Source when opaque and no mask.
  if (color.alpha() == 255u && blendMode == BlendMode::SourceOver && !mask.has_value()) {
    blendMode = BlendMode::Source;
  }

  // Compute memset2d_color for Source mode with no mask.
  std::optional<PremultipliedColorU8> memsetColor;
  if (blendMode == BlendMode::Source && !mask.has_value()) {
    memsetColor = color;
  }

  // Clear is transparent memset (when no mask).
  if (blendMode == BlendMode::Clear && !mask.has_value()) {
    blendMode = BlendMode::Source;
    memsetColor = PremultipliedColorU8::fromRgbaUnchecked(0, 0, 0, 0);
  }

  // Convert color to premultiplied float for pipeline.
  const auto premultColor = PremultipliedColor{
      NormalizedF32::newClamped(static_cast<float>(color.red()) / 255.0f),
      NormalizedF32::newClamped(static_cast<float>(color.green()) / 255.0f),
      NormalizedF32::newClamped(static_cast<float>(color.blue()) / 255.0f),
      NormalizedF32::newClamped(static_cast<float>(color.alpha()) / 255.0f)};

  // Build blit_anti_h pipeline.
  auto blitAntiHRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(premultColor);
    if (mask.has_value()) {
      p.push(Stage::MaskU8);
    }
    if (shouldPreScaleCoverage(blendMode)) {
      p.push(Stage::Scale1Float);
      p.push(Stage::LoadDestination);
      if (const auto stage = toStage(blendMode)) {
        p.push(*stage);
      }
    } else {
      p.push(Stage::LoadDestination);
      if (const auto stage = toStage(blendMode)) {
        p.push(*stage);
      }
      p.push(Stage::Lerp1Float);
    }
    p.push(Stage::Store);
    return p.compile();
  }();

  // Build blit_rect pipeline.
  auto blitRectRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(premultColor);
    if (mask.has_value()) {
      p.push(Stage::MaskU8);
    }
    if (blendMode == BlendMode::SourceOver && !mask.has_value()) {
      p.push(Stage::SourceOverRgba);
    } else {
      if (blendMode != BlendMode::Source) {
        p.push(Stage::LoadDestination);
        if (const auto stage = toStage(blendMode)) {
          p.push(*stage);
        }
      }
      p.push(Stage::Store);
    }
    return p.compile();
  }();

  // Build blit_mask pipeline.
  auto blitMaskRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(premultColor);
    if (mask.has_value()) {
      p.push(Stage::MaskU8);
    }
    if (shouldPreScaleCoverage(blendMode)) {
      p.push(Stage::ScaleU8);
      p.push(Stage::LoadDestination);
      if (const auto stage = toStage(blendMode)) {
        p.push(*stage);
      }
    } else {
      p.push(Stage::LoadDestination);
      if (const auto stage = toStage(blendMode)) {
        p.push(*stage);
      }
      p.push(Stage::LerpU8);
    }
    p.push(Stage::Store);
    return p.compile();
  }();

  return RasterPipelineBlitter(pixmap, false, memsetColor, mask,
                               makeDummyPixmapSrc(),
                               blitAntiHRp, blitRectRp, blitMaskRp);
}

std::optional<RasterPipelineBlitter> RasterPipelineBlitter::createMask(SubPixmapMut* pixmap) {
  if (pixmap == nullptr) {
    return std::nullopt;
  }

  const auto color = Color::white.premultiply();
  const auto memsetColor = color.toColorU8();

  // Build blit_anti_h pipeline (mask mode: lerp dest with coverage).
  auto blitAntiHRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(color);
    p.push(Stage::LoadDestination);
    p.push(Stage::Lerp1Float);
    p.push(Stage::Store);
    return p.compile();
  }();

  // Build blit_rect pipeline (mask mode: overwrite).
  auto blitRectRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(color);
    p.push(Stage::Store);
    return p.compile();
  }();

  // Build blit_mask pipeline (mask mode: lerp dest with mask coverage).
  auto blitMaskRp = [&]() {
    RasterPipelineBuilder p;
    p.pushUniformColor(color);
    p.push(Stage::LoadDestination);
    p.push(Stage::LerpU8);
    p.push(Stage::Store);
    return p.compile();
  }();

  return RasterPipelineBlitter(pixmap, true, memsetColor, std::nullopt,
                               makeDummyPixmapSrc(),
                               blitAntiHRp, blitRectRp, blitMaskRp);
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

  const auto mask_ctx = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();

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
      blit_anti_h_rp_.ctx().current_coverage =
          static_cast<float>(coverage) * (1.0f / 255.0f);
      const auto rect = ScreenIntRect::fromXYWHSafe(x, y, run, 1u);
      blit_anti_h_rp_.run(rect, AAMaskCtx{}, mask_ctx, pixmap_src_ref, pixmap_);
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

  const auto bounds = ScreenIntRect::fromXYWHSafe(x, y, 1u, height);
  const AAMaskCtx aa_mask_ctx{
      {alpha, alpha},
      0u,  // row_bytes=0: reuse same data for all rows
      static_cast<std::size_t>(bounds.x())};
  const auto mask_ctx = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();
  blit_mask_rp_.run(bounds, aa_mask_ctx, mask_ctx, pixmap_src_ref, pixmap_);
}

void RasterPipelineBlitter::blitAntiH2(std::uint32_t x,
                                       std::uint32_t y,
                                       AlphaU8 alpha0,
                                       AlphaU8 alpha1) {
  const auto bounds = ScreenIntRect::fromXYWH(x, y, 2, 1);
  if (!bounds.has_value()) {
    return;
  }
  const AAMaskCtx aa_mask_ctx{
      {alpha0, alpha1},
      2u,
      static_cast<std::size_t>(bounds->x() + bounds->y() * 2)};
  const auto mask_ctx = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();
  blit_mask_rp_.run(*bounds, aa_mask_ctx, mask_ctx, pixmap_src_ref, pixmap_);
}

void RasterPipelineBlitter::blitAntiV2(std::uint32_t x,
                                       std::uint32_t y,
                                       AlphaU8 alpha0,
                                       AlphaU8 alpha1) {
  const auto bounds = ScreenIntRect::fromXYWH(x, y, 1, 2);
  if (!bounds.has_value()) {
    return;
  }
  const AAMaskCtx aa_mask_ctx{
      {alpha0, alpha1},
      1u,
      static_cast<std::size_t>(bounds->x() + bounds->y() * 1)};
  const auto mask_ctx = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();
  blit_mask_rp_.run(*bounds, aa_mask_ctx, mask_ctx, pixmap_src_ref, pixmap_);
}

void RasterPipelineBlitter::blitRect(const ScreenIntRect& rect) {
  if (pixmap_ == nullptr) {
    return;
  }

  if (memset_color_.has_value()) {
    const auto c = *memset_color_;
    const auto maxX = std::min<std::size_t>(pixmap_->width(), rect.x() + rect.width());
    const auto maxY = std::min<std::size_t>(pixmap_->height(), rect.y() + rect.height());
    auto* data = pixmap_->data;

    if (is_mask_only_) {
      for (std::size_t yy = rect.y(); yy < maxY; ++yy) {
        for (std::size_t xx = rect.x(); xx < maxX; ++xx) {
          const auto offset = (yy * pixmap_->real_width + xx) * kBytesPerPixel;
          data[offset + 3] = c.alpha();
        }
      }
    } else {
      for (std::size_t yy = rect.y(); yy < maxY; ++yy) {
        for (std::size_t xx = rect.x(); xx < maxX; ++xx) {
          const auto offset = (yy * pixmap_->real_width + xx) * kBytesPerPixel;
          data[offset + 0] = c.red();
          data[offset + 1] = c.green();
          data[offset + 2] = c.blue();
          data[offset + 3] = c.alpha();
        }
      }
    }
    return;
  }

  const auto mask_ctx = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();
  blit_rect_rp_.run(rect, AAMaskCtx{}, mask_ctx, pixmap_src_ref, pixmap_);
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

  const auto mask_ctx_external = makeMaskCtx(mask_);
  const auto pixmap_src_ref = pixmap_src_storage_.asRef();

  // Process row by row, 2 pixels at a time through the pipeline.
  for (std::size_t yy = clip.y(); yy < maxY; ++yy) {
    const auto maskY = yy - clip.y();
    std::size_t xx = clip.x();
    while (xx < maxX) {
      const auto maskX = xx - clip.x();
      const auto remaining = maxX - xx;
      const auto count = std::min<std::size_t>(remaining, 2u);

      const auto c0 = maskData[maskY * mask.width() + maskX];
      const auto c1 =
          (count > 1 && maskX + 1 < mask.width())
              ? maskData[maskY * mask.width() + maskX + 1]
              : static_cast<std::uint8_t>(0u);

      if (c0 == 0u && c1 == 0u) {
        xx += count;
        continue;
      }

      const auto chunkWidth = static_cast<std::uint32_t>(count);
      const auto chunkRect =
          ScreenIntRect::fromXYWH(static_cast<std::uint32_t>(xx),
                                  static_cast<std::uint32_t>(yy), chunkWidth, 1u);
      if (!chunkRect.has_value()) {
        xx += count;
        continue;
      }

      const AAMaskCtx aa_mask_ctx{
          {c0, c1},
          chunkWidth,
          static_cast<std::size_t>(chunkRect->x() + chunkRect->y() * chunkWidth)};
      blit_mask_rp_.run(*chunkRect, aa_mask_ctx, mask_ctx_external,
                        pixmap_src_ref, pixmap_);
      xx += count;
    }
  }
}

}  // namespace tiny_skia::pipeline
