# Design: Rust-to-C++ Translation Validation Audit

**Status:** Complete
**Author:** Claude
**Created:** 2026-03-02

## Summary

Systematic audit of every C++ file against its Rust counterpart to confirm
line-by-line translation fidelity. The audit proceeds in two phases:

1. **Phase 1 — Inventory**: file-by-file, function-by-function comparison
   cataloguing every structural divergence between Rust and C++.
2. **Phase 2 — Fixing**: ordered work items that bring each divergence to parity,
   with `bazel test //...` passing after every change.

The ultimate goal is that every C++ function reads as a direct, reviewable
transliteration of the corresponding Rust function — same control flow, same
constants, same edge-case branches — differing only in language idioms.

## Goals

- Produce a complete inventory of structural and semantic differences between
  the Rust source and the C++ port.
- Identify C++ abstractions that should be introduced so the code reads more
  like the Rust original (e.g., a `U16x16` SIMD-style wrapper so lowp code
  mirrors Rust's `u16x16` usage).
- Create an ordered fix plan where each work item is independently testable.
- Maintain a green build at every step: `bazel build //...` and `bazel test //...`.

## Non-Goals

- No performance optimization beyond what is needed for translation fidelity.
- No new features or API surface changes.
- No SIMD intrinsics — keep scalar fallback implementations, matching Rust's
  own scalar-fallback paths.
- PNG I/O remains out of scope.

## Next Steps

1. Review and approve this design doc.
2. Begin Phase 2 fixes starting with the wide-type foundation (F32x8T gaps),
   then pipeline structural alignment, then remaining module fixes.
3. After all fixes land, promote function-map statuses from `🟡` → `🟢`.

## Implementation Plan

### Phase 1: Inventory (complete — findings below)

- [x] Inventory wide/ module types vs Rust `wide` crate types
- [x] Inventory pipeline/ (lowp, highp, blitter, mod) structural differences
- [x] Inventory scan/ module structural differences
- [x] Inventory core modules (color, math, fixed_point, geom, edge, etc.)
- [x] Inventory path modules (path, path_builder, stroker, dash, transform)
- [x] Inventory shader modules (gradient, linear, radial, sweep, pattern)
- [x] Inventory path64/ module structural differences
- [x] Catalogue WIP bug fixes in path64 (Cubic64, LineCubicIntersections)

### Phase 2: Fixing (work items below)

- [x] **WI-01**: Add missing F32x8T math methods
- [x] **WI-02**: Add missing U32x4T / U32x8T operators
- [x] **WI-03**: Introduce lowp `LowpChannel` type alias (mirrors Rust `u16x16`)
- [x] **WI-04**: Align lowp load/store signatures with Rust patterns
- [x] **WI-05**: Accepted divergence — documented (see §Accepted Structural Divergences)
- [x] **WI-06**: Add `blend_fn`/`blend_fn2` templates for lowp (mirrors Rust macros)
- [x] **WI-07**: Documented as unnecessary — C++ float arrays serve directly as coordinates
- [x] **WI-08**: Add `F32x2` / `F32x4` path-local vector wrappers (`PathVec.h`)
- [x] **WI-09**: Align `cubeRoot` implementation with Rust Halley method
- [x] **WI-10**: Land path64 WIP bug fixes + regression tests
- [x] **WI-11**: Port remaining Rust inline unit tests (stroker auto_close)
- [x] **WI-12**: Promote function-map statuses to `🟢` after vetting

---

## Phase 1: Inventory Findings

### 1. Wide Module (`src/tiny_skia/wide/`)

#### 1.1 Type Representation

All C++ wide types use `std::array<T, N>` as their backing store. Rust uses
conditional compilation (`cfg_if!`) to select native SIMD types (SSE2, AVX,
NEON) or falls back to plain arrays. Since the C++ port targets scalar
fallback parity, `std::array` is the correct choice — but the API surface
must match Rust's full method set.

| Rust type | C++ type | Backing store | API parity |
|-----------|----------|---------------|------------|
| `f32x4` | `F32x4T` | `std::array<float, 4>` | Complete |
| `f32x8` | `F32x8T` | `std::array<float, 8>` | **4 methods missing** |
| `f32x16` | `F32x16T` | `(F32x8T, F32x8T)` | Complete |
| `i32x4` | `I32x4T` | `std::array<int32_t, 4>` | Complete |
| `i32x8` | `I32x8T` | `std::array<int32_t, 8>` | Complete |
| `u16x16` | `U16x16T` | `std::array<uint16_t, 16>` | Near-complete (`as_slice()` missing) |
| `u32x4` | `U32x4T` | `std::array<uint32_t, 4>` | **Missing `^` operator, limited comparisons** |
| `u32x8` | `U32x8T` | `std::array<uint32_t, 8>` | **Missing `^` operator, limited comparisons** |

#### 1.2 F32x8T Missing Methods (critical for highp pipeline)

| Rust method | C++ equivalent | Status |
|-------------|---------------|--------|
| `f32x8::sqrt()` | `F32x8T::sqrt()` | **Missing** |
| `f32x8::recip_fast()` | `F32x8T::recipFast()` | **Missing** |
| `f32x8::recip_sqrt()` | `F32x8T::recipSqrt()` | **Missing** |
| `f32x8::powf(exp)` | `F32x8T::powf(float)` | **Missing** |
| `f32x8 += f32x8` | `F32x8T::operator+=` | **Missing** |

These are used in highp pipeline stages (gamma, gradient, texture sampling).

#### 1.3 U32x4T / U32x8T Missing Operators

| Rust operator | C++ equivalent | Status |
|---------------|---------------|--------|
| `u32x4 ^ u32x4` | `U32x4T::operator^` | **Missing** |
| `u32x8 ^ u32x8` | `U32x8T::operator^` | **Missing** |
| `u32x4::cmp_ne/lt/le/gt/ge` | additional comparison methods | **Missing** |
| `u32x8::cmp_ne/lt/le/gt/ge` | additional comparison methods | **Missing** |

---

### 2. Pipeline Module (`src/tiny_skia/pipeline/`)

#### 2.1 Lowp Pixel Type

| Aspect | Rust | C++ | Divergence |
|--------|------|-----|------------|
| Pixel type | `u16x16` (16 × `u16`) | `std::array<float, 16>` | **Structural** |
| Value range | Integer [0, 255] | Float [0.0, 255.0] | Semantic match via `div255` |
| div255 | `(v + 255) >> 8` (bitwise) | `floor((v + 255) / 256)` (arithmetic) | **Equivalent but different idiom** |

The C++ lowp pipeline operates on floats in [0, 255] rather than native u16
values. While numerically equivalent (the `div255` formula produces identical
results), the code reads very differently from Rust. Introducing a thin
`U16x16`-style type alias or wrapper for lowp pixel channels would make the
code structurally comparable.

#### 2.2 Load/Store Patterns

| Aspect | Rust | C++ |
|--------|------|-----|
| Load input | `&[PremultipliedColorU8; 16]` structured array | Raw `uint8_t*` + stride + coordinates |
| Store output | `&mut [PremultipliedColorU8; 16]` structured array | Raw `uint8_t*` + stride + coordinates |

Rust uses typed `PremultipliedColorU8` accessors (`.red()`, `.green()`, etc.),
while C++ computes byte offsets manually. The Rust approach is more readable
and less error-prone.

#### 2.3 Tail Handling

| Aspect | Rust | C++ |
|--------|------|-----|
| Strategy | **Dual code paths**: `functions` (full) vs `functions_tail` (tail) | **Single path** with `tail` parameter |
| Dispatch | Pipeline switches between two function arrays at `start()` | Same function handles both via `count` parameter |

Rust avoids branches inside hot load/store stages by having separate function
arrays for full-width and tail-width processing. C++ uses a single function
with a `tail` parameter that controls the loop bound. This is a meaningful
structural divergence.

#### 2.4 Blend Mode Code Generation

Rust uses `blend_fn!` and `blend_fn2!` macros to generate blend mode stage
functions with a common pattern. C++ writes each function manually. The logic
is identical but the Rust macro abstraction makes patterns more visible. A C++
template or macro could improve readability.

#### 2.5 Lowp Helper Functions

| Rust helper | Purpose | C++ equivalent |
|-------------|---------|---------------|
| `split(f32x16) → (u16x16, u16x16)` | Reinterpret f32x16 as two u16x16 halves | **Not present** |
| `join(u16x16, u16x16) → f32x16` | Reinterpret two u16x16 halves as f32x16 | **Not present** |
| `mad(f, m, a)` | Multiply-add for coordinate transforms | Inline in C++ (no named helper) |
| `gather_ix()` (highp) | Compute texture gather indices | **Not present as named function** |

#### 2.6 FMA Contraction

C++ correctly disables FMA contraction (`#pragma clang fp contract(off)`) to
match Rust's software SIMD wrappers that prevent LLVM from fusing multiply-add.
This is a good alignment decision.

#### 2.7 STAGES Array

Both Rust and C++ use a flat function-pointer array indexed by `Stage` enum.
The C++ `lowp::STAGES` array had a missing `Luminosity` entry (now fixed).
Both arrays should be verified to have identical ordering and completeness.

---

### 3. Scan Module (`src/tiny_skia/scan/`)

The scan module has **complete structural parity**. All 43 functions across
5 files are marked `🟢` in the design doc.

| File | Rust functions | C++ functions | Status |
|------|---------------|---------------|--------|
| `mod.rs` / `Mod.cpp` | 2 | 2 | `🟢` all |
| `path.rs` / `Path.cpp` | 11 | 11 | `🟢` all |
| `path_aa.rs` / `PathAa.cpp` | 4 | 4 | `🟢` all |
| `hairline.rs` / `Hairline.cpp` | 13 | 13 | `🟢` all |
| `hairline_aa.rs` / `HairlineAa.cpp` | 16 | 16 | `🟢` all |

Translation patterns are consistent:
- `Option<T>` → `std::optional<T>`
- `match` → `if/else` chains
- `&mut dyn Blitter` → `Blitter&` or `Blitter*`
- Rust lifetimes → RAII / raw pointers
- Rust `impl Drop` → C++ destructors

No work items needed for this module.

---

### 4. Core Modules (`src/tiny_skia/`)

#### 4.1 Fully Verified (`🟢`) — No Changes Needed

| Module | Rust file | C++ file | Functions | Status |
|--------|-----------|----------|-----------|--------|
| AlphaRuns | `alpha_runs.rs` | `AlphaRuns.cpp` | 7 | `🟢` |
| BlendMode | `blend_mode.rs` | `BlendMode.cpp` | 2 | `🟢` |
| Blitter | `blitter.rs` | `Blitter.cpp` | 8 | `🟢` |
| Color | `color.rs` | `Color.cpp` | 30 | `🟢` |
| Edge | `edge.rs` | `Edge.cpp` | 10 | `🟢` |
| EdgeBuilder | `edge_builder.rs` | `EdgeBuilder.cpp` | 7 | `🟢` |
| EdgeClipper | `edge_clipper.rs` | `EdgeClipper.cpp` | 17 | `🟢` |
| FixedPoint | `fixed_point.rs` | `FixedPoint.cpp` | 16 | `🟢` |
| Geom | `geom.rs` | `Geom.cpp` | 20 | `🟢` |
| LineClipper | `line_clipper.rs` | `LineClipper.cpp` | 2 | `🟢` |
| Math | `math.rs` | `Math.cpp` | 4 | `🟢` |
| Mask | `mask.rs` | `Mask.cpp` | 14 | `🟢` |
| PathGeometry | `path_geometry.rs` | `PathGeometry.cpp` | 11 | `🟢` |
| Pixmap | `pixmap.rs` | `Pixmap.cpp` | 23 | `🟢` |

#### 4.2 Implemented but Not Yet Vetted (`🟡`)

| Module | Rust file | C++ file | Functions | Status |
|--------|-----------|----------|-----------|--------|
| Painter | `painter.rs` | `Painter.cpp` | 12 | `🟡` |
| Pipeline/Blitter | `pipeline/blitter.rs` | `pipeline/Blitter.cpp` | 8 | `🟡` |
| Pipeline/Highp | `pipeline/highp.rs` | `pipeline/Highp.cpp` | ~50 | Mixed `🟢`/`🟡` |
| Pipeline/Lowp | `pipeline/lowp.rs` | `pipeline/Lowp.cpp` | ~30 | Mixed `🟢`/`🟡` |
| Pipeline/Mod | `pipeline/mod.rs` | `pipeline/Mod.cpp` | 14 | `🟢` |

---

### 5. Path Modules (`tiny-skia-path`)

#### 5.1 Rust `f32x2_t.rs` and `f32x4_t.rs` — No Direct C++ Equivalents

Rust comment: *"Right now, there are no visible benefits of using SIMD for
f32x2/f32x4. So we don't."* These are thin wrappers over `[f32; 2]` and
`[f32; 4]` used internally by path geometry computations. The C++ code
performs the same operations directly on `Point` or plain floats.

For line-by-line comparability, lightweight `F32x2` and `F32x4` wrappers
could be added (simple structs with operator overloads), so path geometry
code reads more like the Rust original.

#### 5.2 `floating_point.rs` — Scattered Across C++ Headers

Rust has a dedicated module with types (`NormalizedF32`, `NormalizedF32Exclusive`,
`NonZeroPositiveF32`, `FiniteF32`) and utility functions (`f32_as_2s_compliment`,
`is_denormalized`, `classify`). In C++, these types are scattered:
- `NormalizedF32` → `Color.h`
- `NormalizedF32Exclusive` → `Scalar.h`
- `NonZeroPositiveF32` → `Scalar.h`
- `FiniteF32` → `Scalar.h`

This is acceptable — no structural change needed.

#### 5.3 Implemented Path Modules (`🟡` — Tests Exist, Vetting Pending)

| Module | Rust file | C++ file | Status |
|--------|-----------|----------|--------|
| Path | `path.rs` | `Path.h` | `🟡` |
| PathBuilder | `path_builder.rs` | `PathBuilder.cpp` | `🟡` |
| Stroker | `stroker.rs` | `Stroker.cpp` | `🟡` |
| Dash | `dash.rs` | `Dash.cpp` | `🟡` |
| Scalar | `scalar.rs` | `Scalar.h` / `Math.h` | `🟡` |
| Transform | `transform.rs` | `pipeline/Mod.h` | `🟡` |
| PathGeometry (path) | `path_geometry.rs` | `PathGeometry.cpp` | `🟡` |

---

### 6. Shader Modules (`src/tiny_skia/shaders/`)

All shader Rust files are consolidated into `shaders/Mod.cpp` + `shaders/Mod.h`
in C++. All functions are at `🟡` status (implemented and tested, vetting pending).

| Rust file | Functions | C++ location | Status |
|-----------|-----------|-------------|--------|
| `shaders/mod.rs` | 5 | `shaders/Mod.h` | `🟡` |
| `shaders/gradient.rs` | 4 | `shaders/Mod.h` | `🟡` |
| `shaders/linear_gradient.rs` | 4 | `shaders/Mod.h` | `🟡` |
| `shaders/radial_gradient.rs` | 8 | `shaders/Mod.h` | `🟡` |
| `shaders/sweep_gradient.rs` | 4 | `shaders/Mod.h` | `🟡` |
| `shaders/pattern.rs` | 4 | `shaders/Mod.h` | `🟡` |

No structural divergences found — shader code follows Rust logic closely.

---

### 7. Path64 Module (`src/tiny_skia/path64/`)

#### 7.1 Overall Status

Most functions are marked `🟢`. Three bugs have been found and fixed in WIP:

| Bug | File | Issue | Fix |
|-----|------|-------|-----|
| `findInflections` coefficient | `Cubic64.cpp` | Used `ax` instead of `bx` | Changed to `bx * cy - by * cx` |
| `binarySearch` conditional | `Cubic64.cpp` | Inverted `if (!ok)` → `if (ok)` | Corrected branch direction |
| `horizontalIntersect` slicing | `LineCubicIntersections.cpp` | Passed full coords instead of y-offset slice | Pass `coords + 1` for y-axis |

#### 7.2 `cubeRoot` Divergence

| Aspect | Rust | C++ |
|--------|------|-----|
| Implementation | Custom Halley-method iteration with bit-hack seed | `std::cbrt()` (standard library) |
| Precision | Deterministic across platforms | Platform-dependent |

For strict parity, C++ should use the same Halley-method implementation.
The C++ code already has the helper functions (`cbrt5d`, `halleyCbrt3d`,
`cbrtaHalleyd`) in `Mod.cpp` — the top-level `cubeRoot` just needs to call
them instead of `std::cbrt`.

---

### 8. Rust Inline Tests Not Yet Ported

| Rust source file | Inline tests | C++ coverage | Gap |
|------------------|-------------|-------------|-----|
| `color.rs` | 6 tests (premultiply, demultiply, bytemuck) | Partial (ColorTest.cpp) | 2-3 tests |
| `dash.rs` | 2 tests (validation, bug_26) | Partial | 1-2 tests |
| `stroker.rs` | 6 tests (auto_close, cubic, big, one_off) | Partial | 3-4 tests |

All other Rust inline test modules are confirmed covered by existing C++ tests.

---

## Phase 2: Fix Plan

Work items are ordered by dependency (foundations first). Each item must leave
`bazel build //...` and `bazel test //...` green.

### WI-01: Add Missing F32x8T Math Methods

**Files:** `src/tiny_skia/wide/F32x8T.h`, `src/tiny_skia/wide/F32x8T.cpp`,
`src/tiny_skia/wide/tests/F32x8TTest.cpp`

Add the following methods matching Rust scalar-fallback implementations:

| Method | Implementation |
|--------|---------------|
| `sqrt()` | Per-lane `std::sqrt(v[i])` |
| `recipFast()` | Per-lane `1.0f / v[i]` |
| `recipSqrt()` | Per-lane `1.0f / std::sqrt(v[i])` |
| `powf(float exp)` | Per-lane `std::powf(v[i], exp)` |
| `operator+=` | Per-lane `v[i] += rhs[i]` |

**Tests:** Add `F32x8TTest.SqrtReciprocalAndPowfMatchPerLaneScalar`.
**Verify:** `bazel test //src/tiny_skia/wide/tests:F32x8TTest`

### WI-02: Add Missing U32x4T / U32x8T Operators

**Files:** `U32x4T.h/.cpp`, `U32x8T.h/.cpp`, tests

Add:
- `operator^` (bitwise XOR) for both types
- `cmpNe`, `cmpLt`, `cmpLe`, `cmpGt`, `cmpGe` comparison methods

**Tests:** Extend existing test files.
**Verify:** `bazel test //src/tiny_skia/wide/tests:...`

### WI-03: Introduce Lowp `U16x16`-Style Pixel Type Alias

**Files:** `src/tiny_skia/pipeline/Lowp.cpp`

Currently lowp uses `std::array<float, 16>` directly. Add a type alias:
```cpp
using LowpChannel = std::array<float, kStageWidth>;  // Mirrors Rust u16x16
```
with named helper operations (`div255`, `saturatingAdd`, etc.) so the lowp
stage functions read more like the Rust code.

**Tests:** Existing pipeline tests should continue to pass unchanged.
**Verify:** `bazel test //src/tiny_skia/pipeline/tests:...`

### WI-04: Align Lowp Load/Store Signatures with Rust

**Files:** `src/tiny_skia/pipeline/Lowp.cpp`

Refactor `load_8888_lowp` and `store_8888_lowp` to accept structured pixel
data (e.g., `std::span<const PremultipliedColorU8>`) instead of raw byte
pointers with manual offset math. This makes the code more comparable to
Rust's `load_8888(&[PremultipliedColorU8; STAGE_WIDTH], ...)`.

**Tests:** Existing tests must remain green.
**Verify:** `bazel test //src/tiny_skia/pipeline/tests:...`

### WI-05: Align Lowp Tail Handling

**Files:** `src/tiny_skia/pipeline/Lowp.cpp`

Rust uses separate `functions` and `functions_tail` arrays, selected at
`start()` time. The C++ approach (single function with `tail` parameter) is
functionally equivalent but structurally different.

**Decision needed:** This is a larger refactor with risk. Options:
- (a) Keep current approach, document the structural difference.
- (b) Refactor to dual function arrays matching Rust.

**Recommendation:** Option (a) — document as an accepted divergence. The
single-path approach is simpler in C++ and produces identical results.

### WI-06: Add Blend-Mode Helper Templates for Lowp

**Files:** `src/tiny_skia/pipeline/Lowp.cpp`

Rust uses `blend_fn!` / `blend_fn2!` macros to generate blend mode stage
functions. Add equivalent C++ templates or macros to reduce boilerplate and
make patterns more visible.

**Decision needed:** Whether this improves readability enough to justify the
change. Low priority.

### WI-07: Add `split()`/`join()` Helpers for Lowp Coordinates

**Files:** `src/tiny_skia/pipeline/Lowp.cpp`

Rust's lowp coordinate stages use `split()` and `join()` to reinterpret
f32x16 as two u16x16 halves for coordinate math. Add equivalent C++ helpers
so coordinate-transform stages read more like Rust.

**Tests:** Existing lowp gradient/transform tests.
**Verify:** `bazel test //src/tiny_skia/pipeline/tests:...`

### WI-08: Add `F32x2` / `F32x4` Path-Local Vector Wrappers

**Files:** New header `src/tiny_skia/PathVec.h` (or inline in `PathGeometry.h`)

Lightweight wrappers matching Rust's `f32x2_t.rs` and `f32x4_t.rs`:
```cpp
struct F32x2 {
    float x, y;
    // operator+, operator-, operator*, splat(), abs(), min(), max()
};
```

This makes path geometry code (Bezier evaluation, extrema finding) more
comparable to Rust. Low priority since the underlying math is already correct.

### WI-09: Align `cubeRoot` with Rust Halley Method

**Files:** `src/tiny_skia/path64/Mod.cpp`

Change `cubeRoot()` to use the existing Halley-method helpers (`cbrt5d`,
`halleyCbrt3d`, `cbrtaHalleyd`) instead of `std::cbrt()`, matching the Rust
implementation for deterministic cross-platform behavior.

**Tests:** `bazel test //src/tiny_skia/path64/tests:...`

### WI-10: Land Path64 WIP Bug Fixes

**Files:** `Cubic64.cpp`, `LineCubicIntersections.cpp`, `Cubic64Test.cpp`

Three bugs already fixed in WIP (see Inventory §7.1):
1. `findInflections` coefficient: `ax` → `bx`
2. `binarySearch` conditional: `!ok` → `ok`
3. `horizontalIntersect` slicing: full coords → y-offset coords

**Tests:** `bazel test //src/tiny_skia/path64/tests:...`

### WI-11: Port Remaining Rust Inline Unit Tests

**Files:** Various test files

| Source | Tests to port | Target |
|--------|--------------|--------|
| `color.rs` | premultiply round-trip, bytemuck layout | `tests/ColorTest.cpp` |
| `dash.rs` | validation edge cases, bug_26 regression | `tests/PathTest.cpp` or new `tests/DashTest.cpp` |
| `stroker.rs` | auto_close, cubic_big, one_off regressions | `tests/PathTest.cpp` or new `tests/StrokerTest.cpp` |

**Verify:** `bazel test //...`

### WI-12: Promote Function-Map Statuses

After all fixes land, perform a final line-by-line vetting pass on all `🟡`
entries and promote them to `🟢`. Update the function mapping docs:
- `docs/design_docs/tiny-skia_cpp_bootstrap_function_maps/pipeline.md`
- `docs/design_docs/tiny-skia_cpp_bootstrap_function_maps/wide.md`
- `docs/design_docs/tiny-skia_cpp_bootstrap_function_maps/shaders.md`
- `docs/design_docs/tiny-skia_cpp_bootstrap_function_maps/painter.md`

---

## Accepted Structural Divergences

These are intentional differences that do not affect correctness and are
documented here rather than "fixed":

| Divergence | Rust | C++ | Rationale |
|------------|------|-----|-----------|
| Lowp pixel type | `u16x16` native integers | `float` in [0,255] | Numerically equivalent; C++ lacks native u16 SIMD wrapper |
| Lowp tail handling | Dual function arrays | Single path + `tail` param | Simpler in C++, identical results |
| Naming convention | `snake_case` | `lowerCamelCase` | Project convention |
| Error types | `Option<T>` / `Result<T,E>` | `std::optional<T>` | Language idiom |
| Pattern matching | `match` expressions | `if/else` chains | Language idiom |
| Lifetimes | Explicit `'a, 'b` | RAII / raw pointers | Language idiom |
| Trait dispatch | `&mut dyn Trait` | `virtual` base class | Language idiom |
| Enum with data | Rust enum variants | `std::variant<...>` | Language idiom |
| cfg_if! SIMD | Conditional native SIMD | Always scalar fallback | Scope decision (perf out of scope) |
| f32x2/f32x4 (path) | Dedicated wrappers | Direct scalar/Point ops | Low impact, optional WI-08 |

## Testing and Validation

- Every work item must leave `bazel build //...` and `bazel test //...` green.
- New methods (WI-01, WI-02) require new unit tests proving per-lane behavior.
- Structural refactors (WI-03, WI-04, WI-07) must not change test outputs.
- Bug fixes (WI-10) must be verified by updated/new tests.
- Final vetting pass (WI-12) requires reading each C++ function against its
  Rust counterpart and confirming branch-for-branch, constant-for-constant match.

## Security / Privacy

No change to trust boundaries. All inputs remain repository-local source files.
