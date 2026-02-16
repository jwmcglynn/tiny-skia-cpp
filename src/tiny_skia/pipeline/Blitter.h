#pragma once

#include <optional>

#include "tiny_skia/Blitter.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Mask.h"
#include "tiny_skia/Pixmap.h"

namespace tiny_skia::pipeline {

class RasterPipelineBlitter final : public tiny_skia::Blitter {
 public:
  static std::optional<RasterPipelineBlitter> create(PremultipliedColorU8 color,
                                                     SubPixmapMut* pixmap,
                                                     std::optional<SubMaskRef> mask = std::nullopt);
  static std::optional<RasterPipelineBlitter> createMask(SubPixmapMut* pixmap);

  void blitH(std::uint32_t x, std::uint32_t y, LengthU32 width) override;
  void blitAntiH(std::uint32_t x,
                 std::uint32_t y,
                 std::span<std::uint8_t> alpha,
                 std::span<AlphaRun> runs) override;
  void blitV(std::uint32_t x, std::uint32_t y, LengthU32 height, AlphaU8 alpha) override;
  void blitAntiH2(std::uint32_t x, std::uint32_t y, AlphaU8 alpha0, AlphaU8 alpha1) override;
  void blitAntiV2(std::uint32_t x, std::uint32_t y, AlphaU8 alpha0, AlphaU8 alpha1) override;
  void blitRect(const ScreenIntRect& rect) override;
  void blitMask(const Mask& mask, const ScreenIntRect& clip) override;

  [[nodiscard]] bool isMaskOnly() const {
    return is_mask_only_;
  }

 private:
  RasterPipelineBlitter(SubPixmapMut* pixmap,
                        bool isMaskOnly,
                        PremultipliedColorU8 memsetColor,
                        std::optional<SubMaskRef> mask);

  void fillMaskRect(const ScreenIntRect& rect, AlphaU8 alpha);
  void blendMaskRectConstant(const ScreenIntRect& rect, AlphaU8 alpha);

  SubPixmapMut* pixmap_ = nullptr;
  bool is_mask_only_ = false;
  PremultipliedColorU8 memset_color_{};
  std::optional<SubMaskRef> mask_;
};

}  // namespace tiny_skia::pipeline
