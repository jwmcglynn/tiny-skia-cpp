# Design: cfg_if SIMD Parity

**Status:** Design
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

- Add Bazel SIMD mode build setting and transition wrappers for `tiny_skia_lib`.
- Split `wide` internals into backend-specific implementation layers while preserving
  public class APIs.
- Add wasm-target build/test coverage in the same batch as x86/aarch64 work.
- Stand up dual-mode test targets and make them part of the normal validation gate.

## Implementation Plan

- [ ] Milestone 1: Add Bazel SIMD mode config and transitions
  - [ ] Step 1: Add `//bazel/config:simd_mode` build setting with values `native` and
    `scalar`.
  - [ ] Step 2: Add `config_setting` + `select()` wiring for SIMD control defines/copts in
    `src/tiny_skia/wide/BUILD.bazel` and `src/tiny_skia/BUILD.bazel`.
  - [ ] Step 3: Add `bazel/simd_transition.bzl` with a transition rule that sets
    `//bazel/config:simd_mode` on a dependency edge.
  - [ ] Step 4: Add transitioned wrapper targets for
    `//src:tiny_skia_lib_native` and `//src:tiny_skia_lib_scalar`.

- [ ] Milestone 2: Align C++ `wide` backend topology with Rust `cfg_if!`
  - [ ] Step 1: Introduce backend-selection macros and a shared backend contract
    (`scalar`, `x86`, `aarch64`, `wasm`).
  - [ ] Step 2: Move current `std::array` implementations into explicit scalar backend files.
  - [ ] Step 3: Implement modern x86 intrinsics backend with AVX2/FMA baseline (policy:
    last-10-years CPUs; roughly 2016+ deployments).
  - [ ] Step 4: Implement aarch64 NEON paths matching Rust `target_arch = "aarch64"` +
    `target_feature = "neon"`.
  - [ ] Step 5: Implement wasm SIMD backend for `simd128` and `relaxed-simd`
    (not stub-only).

- [ ] Milestone 3: Add dual-mode tests and gate them
  - [ ] Step 1: Add a macro/rule to generate paired test targets (`_native`, `_scalar`) from
    one test definition using transitioned deps.
  - [ ] Step 2: Apply dual-mode coverage to wide tests first, then pipeline/core integration
    tests.
  - [ ] Step 3: Add native/scalar test suites for host toolchains and wasm-target test suite
    for `simd128` parity checks.
  - [ ] Step 4: Wire dual-mode + wasm suites into regular CI/local validation commands.

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

## Alternatives Considered

- Runtime CPU dispatch with one binary and multiple ISA kernels.
  - Rejected for this phase: higher complexity and less direct parity with Rust `cfg_if!`.

## Future Work

- [ ] Add runtime dispatch after compile-time parity is complete and validated.
