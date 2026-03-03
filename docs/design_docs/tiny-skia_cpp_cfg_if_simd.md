# Design: cfg_if SIMD Parity

**Status:** In Progress (M1 complete; M2 scalar extraction complete; M2 aarch64 complete;
M3 host complete)
**Author:** Codex
**Created:** 2026-03-02

## Summary

- Bring Rust `cfg_if!` SIMD backend selection in `third_party/tiny-skia/src/wide/*.rs`
  into scope for the C++ port.
- Keep the current scalar path as a first-class backend and make it selectable for parity
  testing.
- Implement wasm SIMD support (`simd128` and `relaxed-simd`) in this scope, not as follow-up.
- Add Bazel transition-based test coverage so we can run the same tests against:
  - current-platform SIMD backend (`native` mode)
  - forced scalar fallback backend (`scalar` mode)
- Confirm supported ISA scope from Rust source: this is broader than amd64/arm.

## Goals

- Match Rust `cfg_if!` structure for `wide` types (`f32x4`, `f32x8`, `i32x4`, `i32x8`,
  `u32x4`, `u32x8`, `u16x16`) with explicit backend selection.
- Support wasm SIMD lanes used by Rust (`simd128`, `relaxed-simd`) as first-class targets.
- Preserve bit-accurate behavior and keep scalar fallback always available.
- Introduce Bazel build settings + transitions so one `bazel test` invocation can validate
  both `native` SIMD and `scalar` fallback variants.
- Constrain x86 SIMD support to modern CPUs (roughly 2016+; last-10-years policy).
- Keep existing API and call sites stable while backend internals evolve.

## Non-Goals

- No runtime CPU dispatch in this batch (compile-time backend selection only).
- No new ISA families beyond what Rust currently gates in `cfg_if!`.
- No AVX-512/SVE/ARMv7 feature work.
- No pre-2016 x86-specific tuning/compatibility work (SSE2-only/SSE4.1-only hosts are
  out of policy for native SIMD mode).
- No PNG I/O or unrelated rendering feature changes.

## Next Steps

- Complete modern x86 AVX2/FMA backend parity work.
- Implement wasm SIMD (`simd128`, `relaxed-simd`) backend paths.
- Wire wasm SIMD test suites into the normal validation gate.

## Implementation Plan

- [x] Milestone 1: Add Bazel SIMD mode config and transitions
  - [x] Step 1: Add `//bazel/config:simd_mode` build setting with values `native` and
    `scalar`.
  - [x] Step 2: Add `config_setting` + `select()` wiring for SIMD control defines/copts in
    `src/tiny_skia/wide/BUILD.bazel` and `src/tiny_skia/BUILD.bazel`.
  - [x] Step 3: Add `bazel/simd_transition.bzl` with a transition rule that sets
    `//bazel/config:simd_mode` on a dependency edge.
  - [x] Step 4: Add transitioned wrapper targets for
    `//src:tiny_skia_lib_native` and `//src:tiny_skia_lib_scalar`.

- [ ] Milestone 2: Align C++ `wide` backend topology with Rust `cfg_if!`
  - [x] Step 1: Introduce backend-selection macros and a shared backend contract
    (`scalar`, `x86`, `aarch64`, `wasm`).
  - [x] Step 2a: Move shared scalar helpers and `u16x16` operations into explicit
    scalar backend files.
  - [x] Step 2b.1: Move `u16x16`, `f32x4`, `i32x4`, and `u32x4` implementations into
    explicit scalar backend files.
  - [x] Step 2b.2: Move remaining wide-type implementations (`f32x8`, `i32x8`, `u32x8`,
    `f32x16`) into explicit scalar backend files.
  - [x] Step 3: Implement modern x86 intrinsics backend with AVX2/FMA baseline (policy:
    last-10-years CPUs; roughly 2016+ deployments).
    - [x] Step 3a: Add AVX2/FMA x86 backends for `u32x8` operations and x8 bitcast/round
      conversion paths (`f32x8`/`i32x8`) to remove scalar fallbacks in native x86 mode.
    - [x] Step 3b: Route `f32x4`, `i32x4`, and `u32x4` native x86 mode through AVX2 lane
      operations so x4 APIs stop falling back to scalar implementations.
    - [x] Step 3c: Extend wide tests to validate x8 bitcast/conversion semantics under the new
      x86 paths.
    - [x] Step 3d: Fix x86 lowp hot path by enabling AVX2/FMA build flags in native mode,
      adding `u16x16` x86 backend operations, and inlining `u16x16` dispatch to remove
      cross-TU operator call overhead in pipeline code.
  - [x] Step 4: Implement aarch64 NEON paths matching Rust `target_arch = "aarch64"` +
    `target_feature = "neon"`.
    - [x] Step 4a: Implement NEON backend + native dispatch for `f32x4`, `i32x4`, `u32x4`.
    - [x] Step 4b: Extend aarch64-native paths for `f32x8`, `i32x8`, `u32x8`, `u16x16`,
      `f32x16` where Rust composes via x4 lanes.
      - [x] Step 4b.1: Route `f32x8`, `i32x8`, and `u32x8` through composed x4 operations in
        aarch64 native mode.
      - [x] Step 4b.2: Route `f32x16` abs/sqrt through `f32x8` halves.
      - [x] Step 4b.3: Add aarch64-native `u16x16` operations (`min`, `max`, `cmpLe`,
        `add/sub/mul`, `and/or`) aligned with Rust neon gates.
      - [x] Step 4b.4: Remove avoidable NEON lane materialization in lowp hot paths by
        adding fused `u16x16` helpers (`mul+div255`, `mul+mul+add+div255`, source-over) and
        routing lowp blend operations through them.
  - [ ] Step 5: Implement wasm SIMD backend for `simd128` and `relaxed-simd`
    (not stub-only).

- [ ] Milestone 3: Add dual-mode tests and gate them
  - [x] Step 1: Add a macro/rule to generate paired test targets (`_native`, `_scalar`) from
    one test definition using transitioned deps.
  - [x] Step 1a: Transition dual-mode direct dep edges so each test target resolves one SIMD
    configuration and avoids mixed-config link edges.
  - [x] Step 2a: Apply dual-mode coverage to wide tests.
  - [x] Step 2b: Extend dual-mode coverage to pipeline/core integration tests.
  - [ ] Step 3: Add native/scalar test suites for host toolchains and wasm-target test suite
    for `simd128` parity checks.
  - [x] Step 4a: Wire host dual-mode suites into regular local validation commands.
  - [ ] Step 4b: Wire wasm SIMD suite into regular CI/local validation commands.

## Requirements and Constraints

- C++20 and Bazel-first workflow only.
- Keep edits minimal and Rust-traceable by file/type.
- Preserve `TINYSKIA_CPP_BIT_EXACT_MODE=1` behavior in both SIMD and scalar modes.
- Scalar mode must not depend on target ISA flags.
- x86 native SIMD mode targets modern CPUs only (last-10-years policy, ~2016+).

## Proposed Architecture

### Rust cfg_if Scope and Supported Instruction Sets

The Rust source currently gates SIMD through these features/arches in `wide/*.rs`:

| Category | Rust gate pattern | Target scope |
|----------|-------------------|--------------|
| Scalar fallback | `else` path in `cfg_if!` | All targets |
| SSE2 baseline | `target_feature = "sse2"` | `x86`, `x86_64` |
| SSE4.1 ops | `target_feature = "sse4.1"` | `x86`, `x86_64` |
| AVX float lanes | `target_feature = "avx"` | `x86`, `x86_64` |
| AVX2 int lanes | `target_feature = "avx2"` | `x86`, `x86_64` |
| NEON | `target_arch = "aarch64"` + `target_feature = "neon"` | `aarch64` |
| Wasm SIMD | `target_feature = "simd128"` | `wasm32` |
| Wasm relaxed SIMD | `target_feature = "relaxed-simd"` | `wasm32` |

Answer to "is there more than amd64 and arm?": yes.
Rust currently includes `x86` (32-bit) and wasm SIMD (`simd128`/`relaxed-simd`) in addition
to `x86_64` and `aarch64`.

### C++ Support Policy for This Port

| Platform | Policy | Notes |
|----------|--------|-------|
| x86 / x86_64 | Modern CPUs only (roughly 2016+) | Native SIMD backend baseline is AVX2/FMA. |
| aarch64 | Supported | NEON backend aligned to Rust gates. |
| wasm32 | Supported | Implement both `simd128` and `relaxed-simd` paths. |
| Any platform | Supported | Scalar fallback backend remains available for parity/testing. |

### C++ Backend Selection Model

- Keep public vector types (`F32x4T`, `I32x4T`, etc.) unchanged.
- Route each operation through backend-specific inline helpers selected by compile-time mode:
  - `native`: allow ISA-specific backend for current target/toolchain.
  - `scalar`: force `std::array` backend regardless of compiler-provided SIMD macros.
- Preserve operation-level parity with Rust (including cases where Rust still uses scalar logic
  even when SIMD is enabled).

### Bazel Transition Strategy

- Add build setting: `//bazel/config:simd_mode` (`native` | `scalar`).
- Use transition rule on dependency edges to produce two configured variants of
  `//src:tiny_skia_lib`.
- Expose stable labels for test deps:
  - `//src:tiny_skia_lib_native`
  - `//src:tiny_skia_lib_scalar`
- Add macro for dual-mode tests so each selected test is built/run against both variants.

## Security / Privacy

- No new external inputs or trust boundaries are introduced.
- This design changes compute backend selection only.

## Testing and Validation

- Unit tests:
  - Run `wide` tests in both modes via transitioned deps.
  - Add backend-specific correctness tests for edge cases (NaN, signed zero, overflow masks,
    lane bitcasts).
- Integration tests:
  - Run rendering/golden integration tests against both `native` and `scalar` variants.
  - Require pixel output parity between variants where Rust guarantees parity.
- Wasm tests:
  - Add wasm target tests for wide operations and selected pipeline cases under `simd128`.
  - Add parity checks between wasm SIMD output and scalar fallback output.
- Build/test gate per implementation step:
  - `bazel build //...`
  - `bazel test //...`
  - `bazel test //tests:tiny_skia_dual_mode_suite`
  - `bazel test //tests:tiny_skia_wasm_simd_suite`

## Recent Update: 2026-03-03 (Dual-Mode Transition Linkage Hardening)

- Fixed a Bazel analysis failure where dual-mode scalar/native tests linked the same C++ and
  gtest dynamic libraries from mixed configurations (`k8-fastbuild-ST-*` and `k8-fastbuild`).
- Simplified dual-mode test wiring by transitioning every direct `dep` edge in
  `bazel/simd_test.bzl` for both `native` and `scalar` targets.
- Removed special-case label rewrites and no longer require dedicated transitioned wrappers for
  `gtest`, `gtest_main`, or test-utils helper libraries.
- Validation run after this update:
  - `bazel build //...` passed
  - `bazel test //...` passed

## Recent Update: 2026-03-03 (X86 AVX2/FMA Backend Completion)

- Added a dedicated AVX2/FMA backend for `u32x8` operations
  (`src/tiny_skia/wide/backend/X86Avx2FmaU32x8T.h`) covering compare/bitwise/add/bitcast paths.
- Extended existing x86 backends for x8 vectors to reduce scalar fallback boundaries:
  - `f32x8`: added bitcast (`toI32`/`toU32`) and integer conversion (`roundInt`/`truncInt`)
    intrinsics paths.
  - `i32x8`: added bitcast (`toU32`/`toF32`) intrinsics paths.
- Reduced load/materialization overhead in native x86 mode by routing `u32x8` directly through
  AVX2 operations (avoids prior split into two x4 vectors on hot paths).
- Routed x4 APIs (`f32x4`, `i32x4`, `u32x4`) through x86 AVX2 lane operations in native mode to
  avoid scalar fallbacks.
- Added/extended tests for x8 bitcast/conversion semantics:
  - `src/tiny_skia/wide/tests/F32x8TTest.cpp`
  - `src/tiny_skia/wide/tests/I32x8TTest.cpp`
  - `src/tiny_skia/wide/tests/U32x8TTest.cpp`
- Validation run after this update:
  - `bazel build //...` passed
  - `bazel test //...` passed

## Recent Update: 2026-03-03 (X86 Lowp U16x16 Hot-Path Fix)

- Identified two x86 regressions behind near-zero SIMD speedup in benchmark native mode:
  - Native config selected `TINYSKIA_CFG_IF_SIMD_NATIVE`, but x86 compile actions did not
    include `-mavx2/-mfma`, so AVX2/FMA backend branches were compiled out.
  - `pipeline/Lowp.cpp` issued many out-of-line `U16x16T` operator calls across translation
    units, blocking inlining in the primary blend hot path.
- Build-system fix:
  - Added x86-specific native config settings in `bazel/config/BUILD.bazel`.
  - Added native x86 AVX2/FMA copts in `bazel/defs.bzl` so native x86 library compiles define
    `__AVX2__` and `__FMA__`.
- Hot-path fix:
  - Added `src/tiny_skia/wide/backend/X86Avx2FmaU16x16T.h` with AVX2 implementations for
    `u16x16` min/max/compare/blend/arith/bitwise and fused lowp helpers
    (`div255`, `mul+div255`, `mul+mul+add+div255`, source-over).
  - Moved `U16x16T` operation dispatch inline into `src/tiny_skia/wide/U16x16T.h`, so lowp
    operations are now inlinable from `pipeline/Lowp.cpp`.
  - Routed lowp fused helpers in `src/tiny_skia/pipeline/Lowp.cpp` through x86 AVX2 helpers in
    native x86 mode.
  - Verified by object inspection: `Lowp.o` no longer references unresolved
    `U16x16T::{operator*,operator+,...}` symbols.
- Validation run after this update:
  - `bazel build //...` passed
  - `bazel test //...` passed
- Benchmark snapshot (`tests/benchmarks/run_render_perf_compare.sh`, opt mode, x86_64 host):
  - FillPath(512): native `707758 ns` vs scalar `1234439 ns` (`1.744x` native/scalar)
  - FillRect(512): native `610272 ns` vs scalar `1082143 ns` (`1.773x` native/scalar)

## Recent Update: 2026-03-03 (X86 Lowp Tail Specialization + Native X4 SIMD)

- Closed remaining x86/native-vs-NEON parity gaps in hot lowp and x4 vector backends:
  - Added x86 helper fusion parity in `X86Avx2FmaU16x16T.h` so `mul+div255` and
    `mul+mul+add+div255` stay in SIMD registers (no extra lane materialization round-trips).
  - Added lowp tail-specialized stages in `src/tiny_skia/pipeline/Lowp.cpp` for:
    `load_dst`, `store`, `load_dst_u8`, `store_u8`, and `source_over_rgba`.
  - Added `STAGES_TAIL` for lowp and wired `Mod.cpp` to use it for tail programs.
  - Updated `RasterPipeline::run()` to build only the active kind's function tables
    (`High` or `Low`) per invocation.
- Added dedicated native x86 x4 SIMD backends:
  - `src/tiny_skia/wide/backend/X86Avx2FmaF32x4T.h`
  - `src/tiny_skia/wide/backend/X86Avx2FmaI32x4T.h`
  - Routed `F32x4T.cpp` and `I32x4T.cpp` through these x4 intrinsics paths
    (instead of widening/narrowing through x8 helpers).
- Validation run after this update:
  - `bazel build //...` passed
  - `bazel test //...` passed
- Benchmark snapshot (`tests/benchmarks/run_render_perf_compare.sh`, opt mode, x86_64 host):
  - FillPath(512): native `486656 ns` vs scalar `1001063 ns` (`2.057x` native/scalar)
  - FillRect(512): native `378478 ns` vs scalar `888819 ns` (`2.348x` native/scalar)
  - Rust gap reduced from ~5x slower (earlier report) to ~1.5x slower
    (`native/rust`: `0.649x` FillPath, `0.656x` FillRect).

## Recent Update: 2026-03-02 (AArch64 NEON Materialization Audit)

- Fixed `u16x16Div255` branch inversion in
  `src/tiny_skia/wide/backend/Aarch64NeonU16x16T.h` so native AArch64 now executes the NEON
  path (fallback remains scalar).
- Added fused AArch64 NEON helpers to reduce repeated load/store boundaries:
  - `u16x16MulDiv255(lhs, rhs)`
  - `u16x16MulAddDiv255(lhs0, rhs0, lhs1, rhs1)`
  - `u16x16SourceOver(source, dest, source_alpha)`
- Updated lowp pipeline call sites (`premultiply`, `scale_*`, `lerp_*`, source/destination-over
  family, modulate/screen/xor, and `source_over_rgba`) to use fused helpers where formulas match.
- Validation run after this update:
  - `bazel build //...` passed
  - `bazel test //...` passed
- Benchmark snapshot (`tests/benchmarks/run_render_perf_compare.sh`, opt mode, Apple Silicon host):
  - FillPath(512): native `237658 ns` vs scalar `442642 ns` (`1.863x` native/scalar)
  - FillRect(512): native `143342 ns` vs scalar `345637 ns` (`2.411x` native/scalar)

## Recent Update: 2026-03-03 (AArch64 Lowp Pixel Marshal Fast Path Regression Fix)

- Investigated a regression after x86 SIMD expansion where native ARM (`aarch64_neon`) and
  scalar builds were near parity in end-to-end benchmarks despite native backend selection.
- Root cause in lowp hot path: `load_8888_lowp` / `store_8888_lowp` / `load_8_lowp` /
  `store_u8` remained scalar lane-by-lane marshaling, dominating runtime and masking NEON math
  gains.
- Implemented AArch64 NEON-native marshaling in `src/tiny_skia/pipeline/Lowp.cpp`:
  - `vld4q_u8` + widen for RGBA load into `U16x16T`.
  - saturating narrow + `vst4q_u8` for RGBA store (restores clamp-to-255 behavior).
  - vectorized u8 mask load/store paths for full-width stages.
- Benchmark snapshot after fix (`tests/benchmarks/run_render_perf_compare.sh`, opt mode,
  Apple Silicon host):
  - FillPath(512): native `222564 ns` vs scalar `321730 ns` (`1.446x` native/scalar)
  - FillRect(512): native `137527 ns` vs scalar `257156 ns` (`1.870x` native/scalar)

## Alternatives Considered

- Runtime CPU dispatch with one binary and multiple ISA kernels.
  - Rejected for this phase: higher complexity and less direct parity with Rust `cfg_if!`.

## Future Work

- [ ] Add runtime dispatch after compile-time parity is complete and validated.
