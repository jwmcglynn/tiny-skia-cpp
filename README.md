# tiny-skia-cpp

A C++20, Bazel-first port of [tiny-skia](https://github.com/linebender/tiny-skia),
a minimal CPU-only 2D rendering library originally written in Rust.

The goal is a line-by-line, bit-accurate C++ translation focused on correctness
and deterministic rendering.

## Porting Status

| Module | Status | Notes |
|--------|--------|-------|
| Core (12 modules) | Complete | AlphaRuns, BlendMode, Blitter, Color, Edge, EdgeBuilder, EdgeClipper, FixedPoint, Geom, LineClipper, Math, PathGeometry |
| Scan (5 modules) | Complete | Mod, Hairline, HairlineAa, Path, PathAa |
| Path64 (5 modules) | Complete | Mod, Point64, Quad64, Cubic64, LineCubicIntersections |
| Wide/SIMD (7 modules) | In progress | F32x4T, F32x8T, I32x4T, I32x8T, U32x4T, U32x8T |
| Pipeline | Partial | Mod complete; Highp/Lowp in progress; Blitter not started |
| Pixmap | In progress | Slice operations done |
| Mask | In progress | Bootstrap done |
| Shaders | Not started | Gradients, patterns |
| Painter | Not started | Rendering orchestrator |

See `docs/design_docs/` for detailed function-level mapping and milestone tracking.

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

## Project Layout

```
src/tiny_skia/           C++ implementation
  pipeline/              Rendering pipeline stages
  scan/                  Scan conversion
  path64/                64-bit path operations
  wide/                  SIMD vector operations
  tests/                 Unit tests (colocated per module)
docs/design_docs/        Design documents and function maps
third_party/tiny-skia/   Canonical Rust source (reference only)
tools/                   Developer tooling
```

## License

BSD-3-Clause — see [LICENSE](LICENSE) for details.
