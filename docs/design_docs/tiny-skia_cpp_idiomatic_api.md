# Design: Idiomatic C++ Public API

**Status:** Design
**Author:** Claude
**Created:** 2026-03-04

## Summary

The public API currently follows Rust conventions — free functions like
`fillRect(pixmapMut, rect, paint, transform)` that take a `PixmapMut&` as the
first argument. This is unnatural in C++, where the same operation would be a
method: `pixmap.fillRect(rect, paint)`.

This design adds method-based drawing APIs to `Pixmap` and `PixmapMut`, moves
implementation-detail symbols out of the public surface, and adds convenience
factory methods — all without introducing allocations or virtual dispatch.

## Goals

- Drawing operations become methods on `Pixmap` and `PixmapMut`.
- Default `Transform::identity()` parameter so callers don't have to spell it
  out in the common case.
- Hide internal helpers (`DrawTiler`, `isTooBigForMath`, `treatAsHairline`,
  `strokeHairline`) from the public header.
- Add `Path::fromRect()` and `Path::fromCircle()` convenience statics.
- Zero new allocations, zero virtual calls, zero overhead vs current code.
- Existing free-function API remains available (but can be deprecated later).

## Non-Goals

- Changing internal implementation or rendering pipeline.
- Wrapping `Shader` (the `std::variant` alias) in a class — the implicit
  conversion from `Color` already works well.
- Adding builder/fluent patterns to `Stroke` or `Paint` — their public
  aggregate fields are idiomatic C++ already.
- Renaming `PixmapRef`/`PixmapMut` — these names are reasonable for non-owning
  views analogous to `std::span`.
- Removing the free-function API in this change.

## Next Steps

1. Add drawing methods to `Pixmap` and `PixmapMut`.
2. Move internal helpers into a `detail` namespace in `Painter.h`.
3. Add `Path::fromRect()` / `Path::fromCircle()`.
4. Build and test.

## Implementation Plan

- [ ] Milestone 1: Drawing methods on `Pixmap` and `PixmapMut`
  - [ ] Add method declarations to `Pixmap` in `Pixmap.h`
  - [ ] Add method declarations to `PixmapMut` in `Pixmap.h`
  - [ ] Implement methods in `Pixmap.cpp` (thin delegation to free functions)
  - [ ] Build and test gate
- [ ] Milestone 2: Hide implementation details
  - [ ] Move `DrawTiler`, `isTooBigForMath`, `treatAsHairline`,
        `strokeHairline` into `namespace detail` in `Painter.h`
  - [ ] Update internal callers
  - [ ] Build and test gate
- [ ] Milestone 3: Path convenience factories
  - [ ] Add `Path::fromRect(const Rect&)` static method
  - [ ] Add `Path::fromCircle(float cx, float cy, float radius)` static method
  - [ ] Build and test gate
- [ ] Milestone 4: Default transform parameter on free functions
  - [ ] Add `= Transform::identity()` default to `fillRect`, `fillPath`,
        `strokePath`, `drawPixmap` free-function signatures
  - [ ] Verify no ambiguity; build and test gate

## Proposed Architecture

### Before (Rust-style)

```cpp
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
auto pixmap = Pixmap::fromSize(100, 100);

Paint paint;
paint.setColorRgba8(0, 128, 255, 255);

auto rect = Rect::fromXywh(10, 10, 80, 80);
pixmap->fillRect(*rect, paint);

auto path = Path::fromCircle(50, 50, 40);
pixmap->fillPath(*path, paint, FillRule::Winding);
```

Key differences:
- `pixmap->fillRect(...)` instead of `fillRect(mut, ...)`
- No explicit `asMut()` call needed
- No `Transform::identity()` boilerplate
- `Path::fromCircle` instead of `PathBuilder::fromCircle`

### New method signatures

```cpp
// --- Pixmap (non-const methods that operate on owned buffer) ---

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

```cpp
// --- PixmapMut (same methods for non-owning mutable views) ---

class PixmapMut {
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

### Implementation

Each method is a one-liner that delegates to the existing free function:

```cpp
void Pixmap::fillRect(const Rect& rect, const Paint& paint,
                      Transform transform, const Mask* mask) {
  auto mut = asMut();
  tiny_skia::fillRect(mut, rect, paint, transform, mask);
}

void PixmapMut::fillRect(const Rect& rect, const Paint& paint,
                         Transform transform, const Mask* mask) {
  tiny_skia::fillRect(*this, rect, paint, transform, mask);
}
```

### Internal-detail hiding

Move these symbols into `namespace detail`:

| Symbol | Reason |
|--------|--------|
| `DrawTiler` | Implementation of tiled rendering |
| `isTooBigForMath` | Internal path validation |
| `treatAsHairline` | Internal stroke classification |
| `strokeHairline` | Internal hairline rendering |

They remain in `Painter.h` (needed by internal callers in `Painter.cpp`) but
are clearly marked as non-public:

```cpp
namespace detail {
  class DrawTiler { ... };
  bool isTooBigForMath(const Path& path);
  std::optional<float> treatAsHairline(const Paint&, float, Transform);
  void strokeHairline(const Path&, const Paint&, LineCap,
                      std::optional<SubMaskRef>, SubPixmapMut&);
}  // namespace detail
```

### Path convenience factories

```cpp
class Path {
 public:
  /// Creates a rectangular path.
  static Path fromRect(const Rect& rect);

  /// Creates a circular path. Returns nullopt for non-positive radius.
  static std::optional<Path> fromCircle(float cx, float cy, float radius);
};
```

`Path::fromRect` replaces the free function `pathFromRect()`.
`Path::fromCircle` delegates to `PathBuilder::fromCircle()`.

### Circular dependency note

`Pixmap.h` currently does not include `Painter.h`. The new drawing methods on
`Pixmap`/`PixmapMut` need `Paint`, `Path`, `Stroke`, `Mask`, `Transform`,
`FillRule`, etc. Two options:

**Option A: Forward-declare in `Pixmap.h`, implement in `Pixmap.cpp`.**
The method bodies go in `Pixmap.cpp` which includes `Painter.h`. `Pixmap.h`
only needs forward declarations. This avoids circular includes.

**Option B: Add a new header `PixmapDraw.h`.**
Keep `Pixmap.h` pure (data + view) and put drawing methods in a separate header
that includes both `Pixmap.h` and `Painter.h`. Users include `PixmapDraw.h` for
the full API.

**Recommendation: Option A.** Forward declarations are simple and keep the API
in one place. Users include `Pixmap.h` and get everything. The forward
declarations for `Paint`, `Stroke`, `Path`, etc. are straightforward.

## Testing and Validation

- Existing tests continue to pass unchanged (free functions still work).
- Add a small set of integration tests exercising the method-based API
  to verify delegation is correct.
- No golden-image changes expected since behavior is identical.
- Build and test gate: `bazel build //...` and `bazel test //...`.

## Alternatives Considered

**Standalone `Canvas` or `Painter` class:**
A non-owning wrapper like `Canvas(pixmapMut)` with drawing methods. Rejected
because it adds an extra object with no benefit — `Pixmap` and `PixmapMut`
already exist and are the natural target for methods.

**Replace `PixmapRef`/`PixmapMut` with `const Pixmap&`/`Pixmap&`:**
Would break users who create views into external memory (e.g., from a
windowing toolkit's framebuffer). Keep the existing types.

**Wrapping `Shader` variant in a class:**
The `std::variant<Color, LinearGradient, ...>` works well with implicit
conversion from `Color`. Wrapping it adds complexity for little gain.
