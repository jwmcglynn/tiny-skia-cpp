# tiny-skia-cpp

A C++20, Bazel-first port of [tiny-skia](https://github.com/linebender/tiny-skia),
a minimal CPU-only 2D rendering library originally written in Rust.

The goal is a line-by-line, bit-accurate C++ translation focused on correctness
and deterministic rendering.

## Porting Status

Every module has been ported and line-by-line audited against the Rust source.
See `docs/design_docs/tiny-skia_cpp_bootstrap_function_maps/` for per-function
status tables.

| Module | Status | Notes |
|--------|--------|-------|
| Core (14 modules) | Complete | AlphaRuns, BlendMode, Blitter, Color, Edge, EdgeBuilder, EdgeClipper, FixedPoint, Geom, LineClipper, Math, Mask, PathGeometry, Pixmap |
| Path modules | Complete | Path, PathBuilder, PathVec (F32x2/F32x4), Stroker, Dash |
| Scan (5 modules) | Complete | Mod, Hairline, HairlineAa, Path, PathAa |
| Path64 (5 modules) | Complete | Mod, Point64, Quad64, Cubic64, LineCubicIntersections |
| Wide/SIMD (7 types) | Complete | F32x4T, F32x8T, F32x16T, I32x4T, I32x8T, U32x4T, U32x8T, U16x16T |
| Pipeline (4 modules) | Complete | Mod, Highp, Lowp, Blitter |
| Shaders (6 modules) | Complete | Mod, Gradient, LinearGradient, RadialGradient, SweepGradient, Pattern |
| Painter | Complete | fillRect, fillPath, strokePath, drawPixmap, applyMask |

See `docs/design_docs/` for design documents, validation audit, and function-level mapping.

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.30+)
- [Bazel](https://bazel.build/) 7+ (or [Bazelisk](https://github.com/bazelbuild/bazelisk))

## Quick Start

```bash
# Install Bazelisk (if bazel is not already installed)
./tools/env-setup.sh

# Build everything
bazel build //...

# Run all tests
bazel test //...
```

The `env-setup.sh` script accepts optional environment variables:

- `INSTALL_DIR` — where to place the `bazel` binary (default: `/usr/local/bin`)
- `BAZELISK_VERSION` — Bazelisk release tag (default: `v1.25.0`)

## Examples

Ported from the Rust `examples/` directory. Each produces a PNG in the current
working directory:

```bash
bazel run //examples:fill              # Two overlapping cubic Bezier fills
bazel run //examples:stroke            # Dashed star polyline with round caps
bazel run //examples:hairline          # Thin strokes at decreasing widths
bazel run //examples:linear_gradient   # Linear gradient fill on a cubic path
bazel run //examples:mask              # Masked donut ring via even-odd clip
bazel run //examples:pattern           # Repeating triangle pattern in a circle
bazel run //examples:gamma             # Color-space comparison (Linear/sRGB)
bazel run //examples:image_on_image    # Compositing one pixmap onto another
bazel run //examples:large_image       # 20000x20000 stress test with masking
```

## Project Layout

```
src/tiny_skia/           C++ implementation
  pipeline/              Rendering pipeline stages
  scan/                  Scan conversion
  path64/                64-bit path operations
  wide/                  SIMD vector operations
  tests/                 Unit tests (colocated per module)
examples/                Executable examples (C++ ports of Rust examples)
docs/design_docs/        Design documents and function maps
third_party/tiny-skia/   Canonical Rust source (reference only)
tools/                   Developer tooling
```

## License

BSD-3-Clause — see [LICENSE](LICENSE) for details.
