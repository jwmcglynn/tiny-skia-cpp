# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/painter.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `FillRule` enum | `FillRule` (Path.h) | 🟡 | Winding, EvenOdd. Pre-existing. |
| `Paint` struct | `Paint` (Painter.h) | 🟡 | shader, blend_mode, anti_alias, colorspace, force_hq_pipeline. Covered by `PaintTest.*` (4 tests). |
| `Paint::default` | `Paint` default initialization | 🟡 | Black color, SourceOver, anti_alias=true, Linear, force_hq=false. Covered by `PaintTest.DefaultPaint`. |
| `Paint::set_color` | `Paint::setColor()` | 🟡 | Covered by `PaintTest.SetColor`. |
| `Paint::set_color_rgba8` | `Paint::setColorRgba8()` | 🟡 | Covered by `PaintTest.SetColorRgba8`. |
| `Paint::is_solid_color` | `Paint::isSolidColor()` | 🟡 | Uses `std::holds_alternative<Color>`. Covered by `PaintTest.IsSolidColorFalseForGradient`. |
| `DrawTiler` struct | `DrawTiler` (Painter.h) | 🟡 | kMaxDimensions=8191, required(), create(), next(). Covered by `DrawTilerTest.*` (5 tests). |
| `DrawTiler::MAX_DIMENSIONS` | `DrawTiler::kMaxDimensions` | 🟡 | `8192 - 1`. |
| `DrawTiler::required` | `DrawTiler::required()` | 🟡 | Covered by `DrawTilerTest.SmallPixmapDoesNotRequireTiling`, `LargePixmapRequiresTiling`. |
| `DrawTiler::new` | `DrawTiler::create()` | 🟡 | Returns optional. Covered by `DrawTilerTest.*`. |
| `DrawTiler::Iterator::next` | `DrawTiler::next()` | 🟡 | Tile iteration with row-major ordering. Covered by `DrawTilerTest.HorizontalTiling`, `VerticalTiling`, `RectTiling`. |
| `is_too_big_for_math` | `isTooBigForMath()` | 🟡 | SCALAR_MAX * 0.25 threshold. NaN-safe via negated comparison. Covered by `PainterHelpersTest.IsTooBigForMath*`. |
| `treat_as_hairline` | `treatAsHairline()` | 🟡 | Zero-width → 1.0, non-AA → None, fastLen + ave. Covered by `PainterHelpersTest.TreatAsHairline*` (4 tests). |
| `PixmapMut::fill_rect` | `fillRect()` | 🟡 | Identity fast path with scan::fillRect{Aa}, transform path delegates to fillPath. Covered by `FillRectTest.*` (4 tests). |
| `PixmapMut::fill_path` | `fillPath()` | 🟡 | Identity path with tiling support, transform path with path/shader transform. Covered by `FillPathTest.*` (3 tests). |
| `PixmapMut::stroke_path` | `strokePath()` | 🟡 | Implemented: dash, hairline detect, thick stroke via fill. Depends on Stroker/Dash. |
| `PixmapMut::stroke_hairline` | `strokeHairline()` | 🟡 | Dispatches to scan::hairline{_aa}::strokePath. Covered by `StrokeHairlineTest.BasicStroke`. |
| `PixmapMut::draw_pixmap` | `drawPixmap()` | 🟡 | Creates Pattern shader + fillRect. Smoke test only (needs Gather stage). Covered by `DrawPixmapTest.DrawOntoPixmapDoesNotCrash`. |
| `PixmapMut::apply_mask` | `applyMask()` | 🟡 | LoadMaskU8 → LoadDestination → DestinationIn → Store pipeline. Covered by `ApplyMaskTest.*` (2 tests). |

### `third_party/tiny-skia/src/pipeline/blitter.rs` — Paint-aware factory
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `RasterPipelineBlitter::new(paint, mask, pixmap)` | `RasterPipelineBlitter::create(Paint, mask, pixmap)` | 🟡 | Full shader pipeline construction: blit_anti_h_rp, blit_rect_rp, blit_mask_rp. Blend mode optimizations (Destination reject, SourceOver→Source, Source memset, Clear→memset). Pattern pixmap cloning. Covered by all FillRect/FillPath integration tests. |

### Infrastructure additions
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Rect::width` | `Rect::width()` (Geom.h) | 🟡 | Covered by `RectTest.WidthHeight`. |
| `Rect::height` | `Rect::height()` (Geom.h) | 🟡 | Covered by `RectTest.WidthHeight`. |
| `IntSize::to_int_rect` | `IntSize::toIntRect()` (Geom.h) | 🟡 | Covered by `IntSizeTest.ToIntRect`. |
| `IntSize::to_rect` | `IntSize::toRect()` (Geom.h) | 🟡 | Covered by `IntSizeTest.ToRect`. |
| `Transform::map_points` | `Transform::mapPoints()` (pipeline/Mod.h) | 🟡 | Affine point mapping. Covered by `TransformTest.MapPoints*` (3 tests). |
| `Path::transform` | `Path::transform()` (Path.h) | 🟡 | Returns new Path with transformed points. Covered by `PathTest.Transform*` (2 tests). |
| `PathBuilder::from_rect` | `pathFromRect()` (Path.h) | 🟡 | Creates Move-Line-Line-Line-Close path. Covered by `PathHelperTest.PathFromRect`. |
