#pragma once

#include <optional>

#include "tiny_skia/BlendMode.h"
#include "tiny_skia/Blitter.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Mask.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia {
struct Paint;
}

namespace tiny_skia::pipeline {

class RasterPipelineBlitter final : public tiny_skia::Blitter {
 public:
  /// Creates a blitter from a solid color.
  static std::optional<RasterPipelineBlitter> create(PremultipliedColorU8 color,
                                                     SubPixmapMut* pixmap,
                                                     BlendMode blendMode = BlendMode::SourceOver,
                                                     std::optional<SubMaskRef> mask = std::nullopt);

  /// Creates a blitter from a Paint (with Shader support).
  /// Matches Rust `RasterPipelineBlitter::new(paint, mask, pixmap)`.
  static std::optional<RasterPipelineBlitter> create(const tiny_skia::Paint& paint,
                                                     std::optional<SubMaskRef> mask,
                                                     SubPixmapMut* pixmap);

  static std::optional<RasterPipelineBlitter> createMask(SubPixmapMut* pixmap);

  void blitH(std::uint32_t x, std::uint32_t y, LengthU32 width) override;
  void blitAntiH(std::uint32_t x, std::uint32_t y, std::span<std::uint8_t> alpha,
                 std::span<AlphaRun> runs) override;
  void blitV(std::uint32_t x, std::uint32_t y, LengthU32 height, AlphaU8 alpha) override;
  void blitAntiH2(std::uint32_t x, std::uint32_t y, AlphaU8 alpha0, AlphaU8 alpha1) override;
  void blitAntiV2(std::uint32_t x, std::uint32_t y, AlphaU8 alpha0, AlphaU8 alpha1) override;
  void blitRect(const ScreenIntRect& rect) override;
  void blitMask(const Mask& mask, const ScreenIntRect& clip) override;

  [[nodiscard]] bool isMaskOnly() const { return is_mask_only_; }

 private:
  RasterPipelineBlitter(SubPixmapMut* pixmap, bool isMaskOnly,
                        std::optional<PremultipliedColorU8> memsetColor,
                        std::optional<SubMaskRef> mask, Pixmap pixmapSrcStorage,
                        RasterPipeline blitAntiHRp, RasterPipeline blitRectRp,
                        RasterPipeline blitMaskRp);

  SubPixmapMut* pixmap_ = nullptr;
  bool is_mask_only_ = false;
  std::optional<PremultipliedColorU8> memset_color_;
  std::optional<SubMaskRef> mask_;
  Pixmap pixmap_src_storage_;
  RasterPipeline blit_anti_h_rp_;
  RasterPipeline blit_rect_rp_;
  RasterPipeline blit_mask_rp_;
};

}  // namespace tiny_skia::pipeline
