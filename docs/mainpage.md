# tiny-skia-cpp {#mainpage}

A C++20 2D rendering library — a faithful port of [tiny-skia](https://github.com/niclasberg/tiny-skia) (Rust).

## Quick start

```cpp
#include "tiny_skia/Painter.h"
#include "tiny_skia/PathBuilder.h"
#include "tiny_skia/Pixmap.h"

using namespace tiny_skia;

// Create a 500x500 RGBA pixmap (transparent black).
auto pixmap = Pixmap::fromSize(500, 500).value();
auto view = pixmap.mutableView();

// Build a triangle path.
PathBuilder pb;
pb.moveTo(250, 50);
pb.lineTo(450, 400);
pb.lineTo(50, 400);
pb.close();
auto path = pb.finish().value();

// Fill with a semi-transparent green.
Paint paint;
paint.setColorRgba8(0, 200, 80, 180);
Painter::fillPath(view, path, paint, FillRule::Winding);

// pixmap now contains the rendered triangle.
// Call pixmap.releaseDemultiplied() to get straight-alpha RGBA bytes for PNG encoding.
```

## Key types

| Type | Purpose |
|------|---------|
| @ref tiny_skia::Painter | Static drawing methods: fillRect, fillPath, strokePath, drawPixmap, applyMask |
| @ref tiny_skia::Pixmap | Owned RGBA pixel buffer |
| @ref tiny_skia::MutablePixmapView | Non-owning mutable view into a pixmap (drawing target) |
| @ref tiny_skia::Path | Immutable vector path (lines, quads, cubics) |
| @ref tiny_skia::PathBuilder | Builder for constructing Path objects |
| @ref tiny_skia::Paint | Shader + blend mode + anti-alias settings |
| @ref tiny_skia::Transform | 2D affine transformation matrix |
| @ref tiny_skia::Mask | 8-bit alpha mask for clipping |
| @ref tiny_skia::Stroke | Stroke width, line cap, line join, dash pattern |
| @ref tiny_skia::Color | Floating-point RGBA color \[0,1\] |
| @ref tiny_skia::ColorU8 | 8-bit RGBA color |

## Shader types

The @ref tiny_skia::Shader variant holds one of:
- @ref tiny_skia::Color — solid color
- @ref tiny_skia::LinearGradient — two-point linear gradient
- @ref tiny_skia::RadialGradient — two-point conical gradient
- @ref tiny_skia::SweepGradient — angular sweep gradient
- @ref tiny_skia::Pattern — pixmap-based pattern (tiled/clamped)

## Examples

Full examples are in the `examples/` directory:
- @ref fill.cpp — basic path filling
- @ref stroke.cpp — dashed stroke with round caps
- @ref linear_gradient.cpp — linear gradient shader
- @ref mask.cpp — alpha mask clipping
- @ref pattern.cpp — pixmap pattern shader
