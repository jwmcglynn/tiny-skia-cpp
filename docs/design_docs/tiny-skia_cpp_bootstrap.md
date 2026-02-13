# Design: Tiny-Skia C++ Port Bootstrap

**Status:** Design
**Author:** Codex
**Created:** 2026-02-13

## Summary
- Create the first-stage scaffolding for a line-by-line, bit-accurate C++20 port of Rust tiny-skia.
- The initial goal is to establish a Bazel-first workspace and directory structure before any C++ translation starts.
- This design gates implementation sequencing and keeps the migration deterministic and auditable.

## Steering Decisions
- Always run `bazel build //...` after each functional porting step.
- Add/extend C++ tests during porting and validate with `bazel test //...`.
- Add image-regression gates with pixel-diff tests (pixelmatch-cpp) when rendering
  outputs are available for C++ parity runs.
- After user review/approval to proceed, commit current changes before any further
  implementation edits.

## Goals
- Set up a Bazel build system and top-level project layout usable from day one.
- Keep repository layout optimized for incremental porting and easy parity validation.
- Make `third_party/tiny-skia` available in-repo for reference.
- Keep decisions explicit in design docs so later phases can proceed milestone by milestone.

## Non-Goals
- No rendering behavior will be changed yet.
- No C++ implementation of tiny-skia modules is included in this phase.
- No strict performance tuning in this initial setup.
- No test harness parity with tiny-skia test suites yet.

## Next Steps
- Create the Bazel workspace/build skeleton and placeholder targets.
- Add a bootstrap design doc and keep it as the process gate for next implementation tasks.
- Add a project directory layout aligned with bit-accurate porting stages.

## Implementation Plan

### Milestone 0: Alignment and ownership
- [ ] Confirm canonical conventions:
  - Bzlmod-only build (`MODULE.bazel` ownership, no `WORKSPACE` runtime dependency).
  - Bazel package model with `src/tiny_skia/` as library and header home.
  - UpperCamel filenames and `tiny_skia` include root (`#include <tiny_skia/Filename.h>`).
- [ ] Define the file naming and visibility contract:
  - Public header path: `src/tiny_skia/<File>.h`.
  - Private include path only for local includes within the package.
- [ ] Record build and porting risk policy in this design doc.
- [ ] Add this design doc section as the gating checkpoint before every module handoff.

### Milestone 1: Build and repository bootstrap (required before functional ports)
- [ ] Verify `MODULE.bazel` captures all external toolchain dependencies.
- [ ] Finalize `BUILD.bazel` stubs in `src/`, `tests/`, and nested modules.
- [ ] Finalize `bazel/defs.bzl` macro API and document call patterns.
- [ ] Add baseline test target(s) for build smoke checks.
- [ ] Add Bazel outputs and `MODULE.bazel.lock` to `.gitignore`.
- [ ] Ensure `bazel build //...` is green before any C++ translation starts.

### Milestone 2: Rust reference indexing
- [ ] Validate that all Rust files from `third_party/tiny-skia/src` are indexed in the tracker table.
- [ ] For each Rust file, record symbol ownership, dependencies, and visibility assumptions.
- [ ] Mark dependencies between modules in the tracker before code starts moving.
- [ ] Identify hard equivalence anchors (golden vectors, float semantics, bit-level ops).
- [ ] Resolve any module-order blockers by dependency DAG before porting.

### Milestone 3: Translation workflow lock
- [ ] For each Rust file:
  - Add row to the file-level function table with `Rust function/item` / `C++ function/item`.
  - Add status and equivalence check entry for every function.
- [ ] Port in deterministic order: foundations → geometry/path64 → scan/pipeline → shading.
- [ ] Keep headers and sources colocated under `src/tiny_skia` and update BUILD deps as you go.
- [ ] Compile every partially done file through incremental builds.

### Milestone 4: Function-by-function porting execution
- [ ] For each function:
  - Translate signature and types preserving semantics and error behavior.
  - Port constants, helper invariants, and edge-case branches first.
  - Add C++ unit or property tests with strict equality or documented epsilon tolerance.
  - Add a gtest parity test against Rust-derived behavior for each function when feasible.
  - Update tracker status from `☐` to `🟡` then `✅`.
- [ ] For each file, mark blocked status `⏸` with explicit reason if unresolved.
- [ ] If Rust tests are missing for a function, add equivalent C++ coverage first.

### Milestone 5: Validation and release gates
- [ ] Update port tracker and function tables after each file is fully ported.
- [ ] Keep per-file completion criteria:
  - File-level build success.
  - All declared functions have status and test evidence.
- [ ] Add module integration tests for public API seams once adjacent files are complete.
- [ ] Require a clean `bazel build //...` as a gate before moving to next milestone.

## Proposed Architecture
- A thin monorepo-style topology:
  - `third_party/tiny-skia` holds canonical Rust source for reference only.
  - `src/tiny_skia/` holds C++ source and headers, colocated per module.
  - `bazel/` holds shared Bazel macros and eventual repository helpers.
- Bazel will be the primary build driver, with targets structured so each module can be ported and validated in isolation.

### Data Flow for This Phase
- Inputs: Rust reference source in `third_party/tiny-skia`.
- Tooling: Bazel reads package files under `src/tiny_skia/` and `tests/`.
- Outputs: Buildable package graph and reproducible module checkpoints for next implementation milestones.

```mermaid
flowchart LR
    A[third_party/tiny-skia] --> B[Port Plan in docs/design_docs]
    B --> C[Bazel workspace + package targets]
    C --> D[Incremental C++ modules under src/tiny_skia]
    D --> E[Target-by-target parity checkpoints]
```

## Testing and Validation
- No functional test execution is required in this bootstrap milestone.
- Validation of bootstrap includes:
  - Confirm `MODULE.bazel` and package BUILD files are syntactically present.
  - Confirm `third_party/tiny-skia` is checked in locally.
  - Confirm design doc gate remains updated before implementation work continues.

## Porting Tracker (Rust file → C++ file)

Legend: `✅` Ported, `🟡` In progress, `⏸` Blocked, `☐` Not started.

| Old Rust file | New C++ file | Status |
| --- | --- | --- |
| `third_party/tiny-skia/src/lib.rs` | `src/tiny_skia/Lib.cpp` + `src/tiny_skia/Lib.h` | ☐ |
| `third_party/tiny-skia/src/alpha_runs.rs` | `src/tiny_skia/AlphaRuns.cpp` + `src/tiny_skia/AlphaRuns.h` | 🟡 |
| `third_party/tiny-skia/src/blend_mode.rs` | `src/tiny_skia/BlendMode.cpp` + `src/tiny_skia/BlendMode.h` | ☐ |
| `third_party/tiny-skia/src/blitter.rs` | `src/tiny_skia/Blitter.cpp` + `src/tiny_skia/Blitter.h` | ☐ |
| `third_party/tiny-skia/src/color.rs` | `src/tiny_skia/Color.cpp` + `src/tiny_skia/Color.h` | ☐ |
| `third_party/tiny-skia/src/edge.rs` | `src/tiny_skia/Edge.cpp` + `src/tiny_skia/Edge.h` | ☐ |
| `third_party/tiny-skia/src/edge_builder.rs` | `src/tiny_skia/EdgeBuilder.cpp` + `src/tiny_skia/EdgeBuilder.h` | ☐ |
| `third_party/tiny-skia/src/edge_clipper.rs` | `src/tiny_skia/EdgeClipper.cpp` + `src/tiny_skia/EdgeClipper.h` | ☐ |
| `third_party/tiny-skia/src/fixed_point.rs` | `src/tiny_skia/FixedPoint.cpp` + `src/tiny_skia/FixedPoint.h` | ☐ |
| `third_party/tiny-skia/src/geom.rs` | `src/tiny_skia/Geom.cpp` + `src/tiny_skia/Geom.h` | ☐ |
| `third_party/tiny-skia/src/line_clipper.rs` | `src/tiny_skia/LineClipper.cpp` + `src/tiny_skia/LineClipper.h` | ☐ |
| `third_party/tiny-skia/src/math.rs` | `src/tiny_skia/Math.cpp` + `src/tiny_skia/Math.h` | 🟡 |
| `third_party/tiny-skia/src/mask.rs` | `src/tiny_skia/Mask.cpp` + `src/tiny_skia/Mask.h` | ☐ |
| `third_party/tiny-skia/src/path_geometry.rs` | `src/tiny_skia/PathGeometry.cpp` + `src/tiny_skia/PathGeometry.h` | ☐ |
| `third_party/tiny-skia/src/painter.rs` | `src/tiny_skia/Painter.cpp` + `src/tiny_skia/Painter.h` | ☐ |
| `third_party/tiny-skia/src/pixmap.rs` | `src/tiny_skia/Pixmap.cpp` + `src/tiny_skia/Pixmap.h` | ☐ |
| `third_party/tiny-skia/src/pipeline/blitter.rs` | `src/tiny_skia/pipeline/Blitter.cpp` + `src/tiny_skia/pipeline/Blitter.h` | ☐ |
| `third_party/tiny-skia/src/pipeline/highp.rs` | `src/tiny_skia/pipeline/Highp.cpp` + `src/tiny_skia/pipeline/Highp.h` | ☐ |
| `third_party/tiny-skia/src/pipeline/lowp.rs` | `src/tiny_skia/pipeline/Lowp.cpp` + `src/tiny_skia/pipeline/Lowp.h` | ☐ |
| `third_party/tiny-skia/src/pipeline/mod.rs` | `src/tiny_skia/pipeline/Mod.cpp` + `src/tiny_skia/pipeline/Mod.h` | ☐ |
| `third_party/tiny-skia/src/scan/hairline.rs` | `src/tiny_skia/scan/Hairline.cpp` + `src/tiny_skia/scan/Hairline.h` | ☐ |
| `third_party/tiny-skia/src/scan/hairline_aa.rs` | `src/tiny_skia/scan/HairlineAa.cpp` + `src/tiny_skia/scan/HairlineAa.h` | ☐ |
| `third_party/tiny-skia/src/scan/mod.rs` | `src/tiny_skia/scan/Mod.cpp` + `src/tiny_skia/scan/Mod.h` | ☐ |
| `third_party/tiny-skia/src/scan/path.rs` | `src/tiny_skia/scan/Path.cpp` + `src/tiny_skia/scan/Path.h` | ☐ |
| `third_party/tiny-skia/src/scan/path_aa.rs` | `src/tiny_skia/scan/PathAa.cpp` + `src/tiny_skia/scan/PathAa.h` | ☐ |
| `third_party/tiny-skia/src/path64/cubic64.rs` | `src/tiny_skia/path64/Cubic64.cpp` + `src/tiny_skia/path64/Cubic64.h` | ☐ |
| `third_party/tiny-skia/src/path64/line_cubic_intersections.rs` | `src/tiny_skia/path64/LineCubicIntersections.cpp` + `src/tiny_skia/path64/LineCubicIntersections.h` | ☐ |
| `third_party/tiny-skia/src/path64/mod.rs` | `src/tiny_skia/path64/Mod.cpp` + `src/tiny_skia/path64/Mod.h` | ☐ |
| `third_party/tiny-skia/src/path64/point64.rs` | `src/tiny_skia/path64/Point64.cpp` + `src/tiny_skia/path64/Point64.h` | ☐ |
| `third_party/tiny-skia/src/path64/quad64.rs` | `src/tiny_skia/path64/Quad64.cpp` + `src/tiny_skia/path64/Quad64.h` | ☐ |
| `third_party/tiny-skia/src/shaders/gradient.rs` | `src/tiny_skia/shaders/Gradient.cpp` + `src/tiny_skia/shaders/Gradient.h` | ☐ |
| `third_party/tiny-skia/src/shaders/linear_gradient.rs` | `src/tiny_skia/shaders/LinearGradient.cpp` + `src/tiny_skia/shaders/LinearGradient.h` | ☐ |
| `third_party/tiny-skia/src/shaders/mod.rs` | `src/tiny_skia/shaders/Mod.cpp` + `src/tiny_skia/shaders/Mod.h` | ☐ |
| `third_party/tiny-skia/src/shaders/pattern.rs` | `src/tiny_skia/shaders/Pattern.cpp` + `src/tiny_skia/shaders/Pattern.h` | ☐ |
| `third_party/tiny-skia/src/shaders/radial_gradient.rs` | `src/tiny_skia/shaders/RadialGradient.cpp` + `src/tiny_skia/shaders/RadialGradient.h` | ☐ |
| `third_party/tiny-skia/src/shaders/sweep_gradient.rs` | `src/tiny_skia/shaders/SweepGradient.cpp` + `src/tiny_skia/shaders/SweepGradient.h` | ☐ |
| `third_party/tiny-skia/src/wide/f32x16_t.rs` | `src/tiny_skia/wide/F32x16T.cpp` + `src/tiny_skia/wide/F32x16T.h` | ☐ |
| `third_party/tiny-skia/src/wide/f32x4_t.rs` | `src/tiny_skia/wide/F32x4T.cpp` + `src/tiny_skia/wide/F32x4T.h` | ☐ |
| `third_party/tiny-skia/src/wide/f32x8_t.rs` | `src/tiny_skia/wide/F32x8T.cpp` + `src/tiny_skia/wide/F32x8T.h` | ☐ |
| `third_party/tiny-skia/src/wide/i32x4_t.rs` | `src/tiny_skia/wide/I32x4T.cpp` + `src/tiny_skia/wide/I32x4T.h` | ☐ |
| `third_party/tiny-skia/src/wide/i32x8_t.rs` | `src/tiny_skia/wide/I32x8T.cpp` + `src/tiny_skia/wide/I32x8T.h` | ☐ |
| `third_party/tiny-skia/src/wide/mod.rs` | `src/tiny_skia/wide/Mod.cpp` + `src/tiny_skia/wide/Mod.h` | ☐ |
| `third_party/tiny-skia/src/wide/u16x16_t.rs` | `src/tiny_skia/wide/U16x16T.cpp` + `src/tiny_skia/wide/U16x16T.h` | ☐ |
| `third_party/tiny-skia/src/wide/u32x4_t.rs` | `src/tiny_skia/wide/U32x4T.cpp` + `src/tiny_skia/wide/U32x4T.h` | ☐ |
| `third_party/tiny-skia/src/wide/u32x8_t.rs` | `src/tiny_skia/wide/U32x8T.cpp` + `src/tiny_skia/wide/U32x8T.h` | ☐ |

### Naming rule
- `snake_case.rs` -> `UpperCamel.cpp` and `UpperCamel.h`
- `mod.rs` -> `Mod.cpp` and `Mod.h`
- C++ function names are lowerCamelCase.

## Function Mapping Tables

When a file is actively being ported, add a table under this section.

### `third_party/tiny-skia/src/lib.rs`
| Rust function/item | C++ function/item | Status | Equivalence checks |
| --- | --- | --- | --- |
| _to be added_ | _to be added_ | ☐ | _to be added_ |

### `third_party/tiny-skia/src/alpha_runs.rs`
| Rust function/item | C++ function/item | Status | Equivalence checks |
| --- | --- | --- | --- |
| `AlphaRuns::new` | `AlphaRuns::AlphaRuns` | 🟡 | Unit-equivalent construction invariants |
| `AlphaRuns::catch_overflow` | `AlphaRuns::catchOverflow` | 🟡 | Manual checks with `0`, `1`, `255`, `256` |
| `AlphaRuns::is_empty` | `AlphaRuns::isEmpty` | 🟡 | Smoke tests for empty and non-empty states |
| `AlphaRuns::reset` | `AlphaRuns::reset` | 🟡 | Reset re-initializes state at width boundary |
| `AlphaRuns::add` | `AlphaRuns::add` | 🟡 | Manual parity vectors vs Rust reference |
| `AlphaRuns::break_run` | `AlphaRuns::breakRun` | 🟡 | Manual parity vectors vs Rust reference |
| `AlphaRuns::break_at` | `AlphaRuns::breakAt` | 🟡 | Manual parity vectors vs Rust reference |

### `third_party/tiny-skia/src/color.rs`
| Rust function/item | C++ function/item | Status | Equivalence checks |
| --- | --- | --- | --- |
| _to be added_ | _to be added_ | ☐ | _to be added_ |

### `third_party/tiny-skia/src/math.rs`
| Rust function/item | C++ function/item | Status | Equivalence checks |
| --- | --- | --- | --- |
| `bound` | `bound` | 🟡 | Compare against min, max, and mid-interval samples |
| `left_shift` | `leftShift` | 🟡 | Bit-equality for positive and negative inputs |
| `left_shift64` | `leftShift64` | 🟡 | Bit-equality for positive and negative inputs |
| `approx_powf` | `approxPowf` | 🟡 | Compare against Rust reference at canonical power pairs |

Add one section per file as soon as implementation begins.

## Security / Privacy
- Inputs are repository-local source files and compiler/runtime dependencies.
- Trust boundary is local workspace state only; no user-provided binary assets are executed during bootstrap.
- Repository hygiene: lock external source fetch to explicit commands and commit `third_party` vendor location.
