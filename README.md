# tiny-skia-cpp

A C++20 port of [tiny-skia](https://github.com/linebender/tiny-skia),
a minimal CPU-only 2D rendering library originally written in Rust.
Builds with Bazel or CMake.

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

## Architecture

The rendering pipeline flows through five major subsystems:

```
PathBuilder → Path → EdgeBuilder → Scan → Pipeline → Pixmap
                                     ↑         ↑
                                   path64    shaders
                                              wide
```

1. **Core** (`src/tiny_skia/`) — fundamental types (Pixmap, Color, Path, Transform,
   Mask, Geom) and algorithms (edge building, clipping, fixed-point math).
2. **Scan** (`src/tiny_skia/scan/`) — converts edges into scanline spans, handling
   both anti-aliased and non-anti-aliased rasterization for fills and hairlines.
3. **Pipeline** (`src/tiny_skia/pipeline/`) — executes a sequence of rendering stages
   (blend, composite, sample, store) on scanline spans. Split into high-precision
   (`Highp`) and low-precision (`Lowp`) fast paths.
4. **Shaders** (`src/tiny_skia/shaders/`) — push sampling stages into the pipeline
   for solid colors, linear/radial/sweep gradients, and pixmap patterns.
5. **Wide** (`src/tiny_skia/wide/`) — SIMD vector wrappers (F32x4T, F32x8T, I32x4T,
   etc.) used by the pipeline for data-parallel execution.
6. **Path64** (`src/tiny_skia/path64/`) — 64-bit path math for precision-sensitive
   intersection and subdivision operations.

The top-level drawing API lives in `Painter.h`, which orchestrates the full pipeline:
`fillRect`, `fillPath`, `strokePath`, `drawPixmap`, and `applyMask`.

### SIMD Backends

The `wide/` layer provides three compile-time backends with an identical API:

| Backend | Platforms | Define |
|---------|-----------|--------|
| **x86 AVX2+FMA** | x86_64 (Intel/AMD) | `TINYSKIA_CFG_IF_SIMD_NATIVE` |
| **ARM64 NEON** | Apple Silicon, AArch64 | `TINYSKIA_CFG_IF_SIMD_NATIVE` |
| **Scalar** | All platforms (portable fallback) | `TINYSKIA_CFG_IF_SIMD_SCALAR` |

Backend selection happens at compile time via `wide/backend/BackendConfig.h`.
The Bazel build uses config transitions (`bazel/simd_transition.bzl`) to produce
paired native/scalar library and test variants automatically. The CMake build
produces two static libraries: `tiny_skia` (native) and `tiny_skia_scalar`.

All backends produce bit-exact results (enforced by `-ffp-contract=off`).

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.30+)
- [Bazel](https://bazel.build/) 7+ (or [Bazelisk](https://github.com/bazelbuild/bazelisk)), **or** [CMake](https://cmake.org/) 3.16+

## Quick Start (Bazel)

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

## Quick Start (CMake)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces two static library targets:

- **`tiny_skia`** — native SIMD (AVX2+FMA on x86_64, NEON on ARM64)
- **`tiny_skia_scalar`** — portable scalar-only backend

To use in your own CMake project:

```cmake
add_subdirectory(path/to/tiny-skia-cpp)
target_link_libraries(your_target PRIVATE tiny_skia)
```

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

## Module Map

| Directory | Responsibility | Key Types / Files |
|-----------|---------------|-------------------|
| `src/tiny_skia/` | Core types and algorithms | Pixmap, Color, Path, Transform, Mask, Geom, Edge, Blitter |
| `src/tiny_skia/scan/` | Scanline rasterization | Hairline, HairlineAa, Path, PathAa |
| `src/tiny_skia/pipeline/` | Rendering pipeline stages | RasterPipelineBuilder, Highp, Lowp, Blitter |
| `src/tiny_skia/shaders/` | Fill shaders and gradients | LinearGradient, RadialGradient, SweepGradient, Pattern |
| `src/tiny_skia/wide/` | SIMD vector wrappers | F32x4T, F32x8T, I32x4T, U32x4T, U32x8T, U16x16T |
| `src/tiny_skia/wide/backend/` | Platform-specific SIMD implementations | Scalar*, X86Avx2Fma*, Aarch64Neon* |
| `src/tiny_skia/path64/` | 64-bit path math | Point64, Quad64, Cubic64, LineCubicIntersections |
| `src/tiny_skia/tests/` | Unit tests (colocated per module) | *Test.cpp files |
| `examples/` | Runnable C++ examples (PNG output) | fill, stroke, gradient, mask, pattern, ... |
| `tests/integration/` | Golden-image integration tests | FillTest, StrokeTest, GradientsTest, ... |
| `tests/benchmarks/` | Performance benchmarks (native vs scalar vs Rust) | RenderPerfBench |
| `tests/rust_ffi/` | Rust FFI for cross-validation against original tiny-skia | tiny_skia_ffi |
| `bazel/` | Build config and SIMD transitions | simd_transition.bzl, simd_test.bzl, defs.bzl |
| `docs/design_docs/` | Design documents and function maps | — |
| `third_party/tiny-skia/` | Canonical Rust source (reference only) | — |
| `tools/` | Developer tooling | env-setup.sh |

## Benchmarks

The benchmark suite compares rendering throughput across C++ native SIMD, C++ scalar,
and the original Rust tiny-skia via FFI:

```bash
# Run individual benchmarks
bazel run -c opt //tests/benchmarks:render_perf_bench_native
bazel run -c opt //tests/benchmarks:render_perf_bench_scalar

# Run comparison summary (prints speedup ratios)
./tests/benchmarks/run_render_perf_compare.sh

# Run regression guard test (enforces architecture-specific watermarks)
bazel test -c opt //tests/benchmarks:render_perf_regression_test
```

See `tests/benchmarks/README.md` for details on metrics and watermark thresholds.

## SIMD Testing

Every test target automatically runs in both native SIMD and scalar modes using the
`tiny_skia_dual_mode_cc_test` Bazel macro (defined in `bazel/simd_test.bzl`). For
each test `foo`, the macro creates:

- `foo_native` — compiled with the platform's native SIMD backend
- `foo_scalar` — compiled with the portable scalar backend
- `foo` — a test suite containing both

```bash
# Run all tests in both modes
bazel test //...

# Run a specific test in both modes
bazel test //src/tiny_skia/tests:tiny_skia_core_tests

# Force a specific mode for all tests
bazel test --//bazel/config:simd_mode=scalar //...
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development workflow, toolchain versions,
formatting, and troubleshooting guidance.

## License

BSD-3-Clause — see [LICENSE](LICENSE) for details.
