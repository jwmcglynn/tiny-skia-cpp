# Design: Idiomatic C++ Public API

**Status:** Design
**Author:** Claude
**Created:** 2026-03-04

## Summary

The public API currently follows Rust conventions — free functions like
`fillRect(pixmapMut, rect, paint, transform)` in the `tiny_skia` namespace.
This is unnatural in C++, where the same operations belong as methods on a
class.

This design:
1. Moves the free drawing functions into a `Painter` class as static methods.
2. Adds convenience instance methods on `Pixmap` and `PixmapMut` that delegate
   to `Painter`.
3. Hides implementation-detail symbols from the public surface.
4. Adds `Path::fromRect()` and `Path::fromCircle()` convenience statics.

All changes are zero-overhead: no allocations, no virtual dispatch.

## Goals

- Free drawing functions become `Painter::fillRect(...)` etc. (static methods).
- `Pixmap`/`PixmapMut` get instance methods as syntactic sugar.
- Default `Transform::identity()` parameter eliminates boilerplate.
- Internal helpers (`DrawTiler`, `isTooBigForMath`, `treatAsHairline`,
  `strokeHairline`) move into `namespace detail` or become private.
- Add `Path::fromRect()` and `Path::fromCircle()` convenience statics.
- Zero new allocations, zero virtual calls, zero overhead vs current code.

## Non-Goals

- Changing internal implementation or rendering pipeline.
- Wrapping `Shader` (the `std::variant` alias) in a class — the implicit
  conversion from `Color` already works well.
- Adding builder/fluent patterns to `Stroke` or `Paint` — their public
  aggregate fields are idiomatic C++ already.
- Renaming `PixmapRef`/`PixmapMut` — these names are reasonable for non-owning
  views analogous to `std::span`.

## Next Steps

1. Introduce `Painter` class with static methods in `Painter.h`.
2. Move internal helpers into `Painter` as private statics or `detail`.
3. Add drawing methods to `Pixmap` and `PixmapMut`.
4. Add `Path::fromRect()` / `Path::fromCircle()`.
5. Build and test.

## Implementation Plan

- [ ] Milestone 1: `Painter` class with static methods
  - [ ] Wrap existing free functions as `Painter::fillRect`,
        `Painter::fillPath`, `Painter::strokePath`, `Painter::drawPixmap`,
        `Painter::applyMask` — all `static`
  - [ ] Move `DrawTiler`, `isTooBigForMath`, `treatAsHairline`,
        `strokeHairline` into `Painter` as `private` statics
        (or a `detail` namespace)
  - [ ] Move `Paint` and `PixmapPaint` out of `Painter.h` into their own
        header `Paint.h` (they are value types, not painter internals)
  - [ ] Remove old free functions entirely; migrate all call sites (~180)
        to `Painter::*`
  - [ ] Add default `Transform` parameter:
        `Transform transform = Transform::identity()`
  - [ ] Build and test gate
- [ ] Milestone 2: Instance methods on `Pixmap` and `PixmapMut`
  - [ ] Add drawing method declarations to `Pixmap` in `Pixmap.h`
  - [ ] Add drawing method declarations to `PixmapMut` in `Pixmap.h`
  - [ ] Implement in `Pixmap.cpp` (thin delegation to `Painter::*`)
  - [ ] Build and test gate
- [ ] Milestone 3: Path convenience factories
  - [ ] Add `Path::fromRect(const Rect&)` static method
  - [ ] Add `Path::fromCircle(float cx, float cy, float radius)` static
  - [ ] Remove `pathFromRect()` free function; migrate all call sites
  - [ ] Build and test gate

## Proposed Architecture

### Before (Rust-style)

```cpp
#include "tiny_skia/Painter.h"

auto pixmap = Pixmap::fromSize(100, 100);
auto mut = pixmap->asMut();

Paint paint;
paint.setColorRgba8(0, 128, 255, 255);

auto rect = Rect::fromXywh(10, 10, 80, 80);
fillRect(mut, *rect, paint, Transform::identity());

auto path = PathBuilder::fromCircle(50, 50, 40);
fillPath(mut, *path, paint, FillRule::Winding, Transform::identity());
```

### After (idiomatic C++)

```cpp
#include "tiny_skia/Painter.h"

auto pixmap = Pixmap::fromSize(100, 100);

Paint paint;
paint.setColorRgba8(0, 128, 255, 255);

// Option A: Static methods on Painter (explicit about which subsystem)
auto rect = Rect::fromXywh(10, 10, 80, 80);
Painter::fillRect(pixmap->asMut(), *rect, paint);

auto path = Path::fromCircle(50, 50, 40);
Painter::fillPath(pixmap->asMut(), *path, paint, FillRule::Winding);

// Option B: Instance methods on Pixmap (most concise)
pixmap->fillRect(*rect, paint);
pixmap->fillPath(*path, paint, FillRule::Winding);
```

Key differences:
- `Painter::fillRect(...)` groups drawing operations under a class
- `pixmap->fillRect(...)` is available as sugar for the common case
- No `Transform::identity()` boilerplate (defaulted)
- `Path::fromCircle` instead of `PathBuilder::fromCircle`

### Painter class

```cpp
/// Groups all drawing operations. All methods are static — Painter has no
/// state and cannot be instantiated. This replaces the previous free
/// functions and provides a clear public API surface.
class Painter {
 public:
  Painter() = delete;

  /// Fills an axis-aligned rectangle onto the pixmap.
  static void fillRect(PixmapMut& pixmap, const Rect& rect,
                       const Paint& paint,
                       Transform transform = Transform::identity(),
                       const Mask* mask = nullptr);

  /// Fills a path onto the pixmap.
  static void fillPath(PixmapMut& pixmap, const Path& path,
                       const Paint& paint, FillRule fillRule,
                       Transform transform = Transform::identity(),
                       const Mask* mask = nullptr);

  /// Strokes a path onto the pixmap.
  static void strokePath(PixmapMut& pixmap, const Path& path,
                         const Paint& paint, const Stroke& stroke,
                         Transform transform = Transform::identity(),
                         const Mask* mask = nullptr);

  /// Composites a source pixmap onto a destination pixmap.
  static void drawPixmap(PixmapMut& pixmap, std::int32_t x,
                         std::int32_t y, PixmapRef src,
                         const PixmapPaint& paint = {},
                         Transform transform = Transform::identity(),
                         const Mask* mask = nullptr);

  /// Applies a mask to already-drawn content.
  static void applyMask(PixmapMut& pixmap, const Mask& mask);

 private:
  // Internal helpers — no longer on the public surface.
  static bool isTooBigForMath(const Path& path);
  static std::optional<float> treatAsHairline(const Paint& paint,
                                              float strokeWidth,
                                              Transform ts);
  static void strokeHairline(const Path& path, const Paint& paint,
                             LineCap lineCap,
                             std::optional<SubMaskRef> mask,
                             SubPixmapMut& subpix);
};
```

`DrawTiler` moves into `namespace detail` (it's a multi-line class, awkward
as a private nested class, and tests may want to unit-test it):

```cpp
namespace detail {
class DrawTiler { /* unchanged implementation */ };
}  // namespace detail
```

### Paint extraction

`Paint` and `PixmapPaint` are currently defined in `Painter.h`. They are
standalone value types with no dependency on `Painter`, so they move to a
new `Paint.h` header. `Painter.h` includes `Paint.h`. Existing code that
includes `Painter.h` continues to compile unchanged.

```
// New file: Paint.h
struct Paint { ... };      // moved from Painter.h
struct PixmapPaint { ... }; // if it exists, also moved
```

### Pixmap / PixmapMut instance methods

```cpp
class Pixmap {
 public:
  // ... existing API unchanged ...

  void fillRect(const Rect& rect, const Paint& paint,
                Transform transform = Transform::identity(),
                const Mask* mask = nullptr);

  void fillPath(const Path& path, const Paint& paint,
                FillRule fillRule,
                Transform transform = Transform::identity(),
                const Mask* mask = nullptr);

  void strokePath(const Path& path, const Paint& paint,
                  const Stroke& stroke,
                  Transform transform = Transform::identity(),
                  const Mask* mask = nullptr);

  void drawPixmap(std::int32_t x, std::int32_t y, PixmapRef src,
                  const PixmapPaint& paint = {},
                  Transform transform = Transform::identity(),
                  const Mask* mask = nullptr);

  void applyMask(const Mask& mask);
};
```

`PixmapMut` gets the same set.

### Implementation (all thin wrappers)

```cpp
// Pixmap delegates through asMut() → Painter
void Pixmap::fillRect(const Rect& rect, const Paint& paint,
                      Transform transform, const Mask* mask) {
  auto mut = asMut();
  Painter::fillRect(mut, rect, paint, transform, mask);
}

// PixmapMut delegates directly to Painter
void PixmapMut::fillRect(const Rect& rect, const Paint& paint,
                         Transform transform, const Mask* mask) {
  Painter::fillRect(*this, rect, paint, transform, mask);
}
```

### Call-site migration

All ~180 call sites are updated in the same change. No deprecated wrappers.
The free functions (`fillRect`, `fillPath`, `strokePath`, `drawPixmap`,
`applyMask`, `strokeHairline`, `pathFromRect`) are removed entirely.

Migration patterns:

| Before | After |
|--------|-------|
| `fillRect(mut, *rect, paint, Transform::identity())` | `Painter::fillRect(mut, *rect, paint)` |
| `fillPath(mut, path, paint, FillRule::Winding, ts)` | `Painter::fillPath(mut, path, paint, FillRule::Winding, ts)` |
| `strokePath(mut, path, paint, stroke, ts)` | `Painter::strokePath(mut, path, paint, stroke, ts)` |
| `drawPixmap(mut, x, y, src, ppaint, ts)` | `Painter::drawPixmap(mut, x, y, src, ppaint, ts)` |
| `applyMask(mut, mask)` | `Painter::applyMask(mut, mask)` |
| `pathFromRect(*rect)` | `Path::fromRect(*rect)` |

Where `Transform::identity()` was the only transform argument, it can now
be omitted entirely thanks to the default parameter.

### Path convenience factories

```cpp
class Path {
 public:
  /// Creates a rectangular path.
  static Path fromRect(const Rect& rect);

  /// Creates a circular path. Returns nullopt for non-positive radius.
  static std::optional<Path> fromCircle(float cx, float cy, float r);
};
```

`Path::fromRect` replaces the free function `pathFromRect()`.
`Path::fromCircle` delegates to `PathBuilder::fromCircle()`.

### Circular dependency note

`Pixmap.h` currently does not include `Painter.h`. The new drawing methods
on `Pixmap`/`PixmapMut` need `Paint`, `Path`, `Stroke`, `Mask`, etc.

**Approach: Forward-declare in `Pixmap.h`, implement in `Pixmap.cpp`.**
The method bodies go in `Pixmap.cpp` which includes `Painter.h`. `Pixmap.h`
only needs forward declarations for `Paint`, `PixmapPaint`, `Path`, `Stroke`,
`Mask`, `Rect`, `FillRule`, `Transform`. This avoids circular includes.

## File changes summary

| File | Change |
|------|--------|
| `Paint.h` (new) | `Paint`, `PixmapPaint` extracted from `Painter.h` |
| `Painter.h` | `Painter` class with static methods; `detail::DrawTiler`; no free functions; includes `Paint.h` |
| `Painter.cpp` | Rename free functions to `Painter::` static methods |
| `Pixmap.h` | Forward declarations + drawing method declarations |
| `Pixmap.cpp` | Drawing method implementations delegating to `Painter` |
| `Path.h` | Add `Path::fromRect()`, `Path::fromCircle()` statics |
| `Path.cpp` | Implement the new statics |

## Testing and Validation

- All existing tests updated to use `Painter::*` methods.
- Add a small set of tests exercising `pixmap.fillRect(...)` instance
  methods to verify delegation.
- No golden-image changes expected since behavior is identical.
- Build and test gate: `bazel build //...` and `bazel test //...`.

## Alternatives Considered

**Free functions only (status quo):**
Works, but is a Rust-ism. Doesn't group related operations and pollutes
the namespace. No discoverability via IDE autocomplete on the class.

**Standalone `Canvas` class wrapping `PixmapMut`:**
A non-owning wrapper like `Canvas(pixmapMut)` with drawing methods. Rejected
because `Painter` with static methods achieves the same grouping without
requiring construction of a wrapper object. Instance methods on `Pixmap`
cover the convenience case.

**Replace `PixmapRef`/`PixmapMut` with `const Pixmap&`/`Pixmap&`:**
Would break users who create views into external memory (e.g., from a
windowing toolkit's framebuffer). Keep the existing types.

**Wrapping `Shader` variant in a class:**
The `std::variant<Color, LinearGradient, ...>` works well with implicit
conversion from `Color`. Wrapping it adds complexity for little gain.
