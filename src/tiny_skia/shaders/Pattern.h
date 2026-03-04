#pragma once

#include "tiny_skia/BlendMode.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia {

// Filter quality for pattern sampling.
enum class FilterQuality {
  Nearest,
  Bilinear,
  Bicubic,
};

// Paint settings for pixmap drawing.
struct PixmapPaint {
  float opacity = 1.0f;
  BlendMode blendMode = BlendMode::SourceOver;
  FilterQuality quality = FilterQuality::Nearest;
};

// A pattern shader (pixmap-based).
class Pattern {
 public:
  Pattern(PixmapRef pixmap, SpreadMode spreadMode, FilterQuality quality, float opacity,
          Transform transform);

  [[nodiscard]] bool isOpaque() const;

  [[nodiscard]] bool pushStages(ColorSpace cs, pipeline::RasterPipelineBuilder& p) const;

  PixmapRef pixmap_;
  NormalizedF32 opacity_;
  Transform transform_;

 private:
  FilterQuality quality_;
  SpreadMode spread_mode_;
};

}  // namespace tiny_skia
