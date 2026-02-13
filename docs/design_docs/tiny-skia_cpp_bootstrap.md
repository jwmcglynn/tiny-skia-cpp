# Design: Tiny-Skia C++ Port Bootstrap

**Status:** Design
**Author:** Codex
**Created:** 2026-02-13

## Summary
- Create the first-stage scaffolding for a line-by-line, bit-accurate C++20 port of Rust tiny-skia.
- The initial goal is to establish a Bazel-first workspace and directory structure before any C++ translation starts.
- This design gates implementation sequencing and keeps the migration deterministic and auditable.

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

- [ ] Milestone 1: Establish build and project skeleton
  - [ ] Add Bazel workspace entrypoint files (`WORKSPACE.bazel`, `BUILD.bazel`).
  - [ ] Add initial Bazel package skeletons in `src/`, `include/`, and `tests/`.
  - [ ] Add Bazel helper macro file `bazel/defs.bzl` for C++ target consistency.
  - [ ] Add top-level `AGENTS.md` and `docs/design_docs/AGENTS.md` to define local conventions.
- [ ] Milestone 2: Reference the upstream Rust implementation in-repo
  - [ ] Clone `tiny-skia` into `third_party/tiny-skia` and lock to a shallow snapshot.
  - [ ] Add notes on source modules and parity mapping strategy in the design doc.
- [ ] Milestone 3: Lock first C++ porting seam
  - [ ] Define the first module boundary (e.g., `path` or `pixmap`) for initial translation.
  - [ ] Add minimal C++ target and buildable “hello” check to ensure compiler/toolchain is wired.
  - [ ] Add initial mapping notes in `docs/design_docs` from Rust symbols to C++ modules.

## Proposed Architecture
- A thin monorepo-style topology:
  - `third_party/tiny-skia` holds canonical Rust source for reference only.
  - `src/` holds C++ translation units by module.
  - `include/` holds exported C++ headers.
  - `bazel/` holds shared Bazel macros and eventual repository helpers.
- Bazel will be the primary build driver, with targets structured so each module can be ported and validated in isolation.

### Data Flow for This Phase
- Inputs: Rust reference source in `third_party/tiny-skia`.
- Tooling: Bazel reads package files under `src/`, `include/`, and `tests/`.
- Outputs: Buildable package graph and reproducible module checkpoints for next implementation milestones.

```mermaid
flowchart LR
    A[third_party/tiny-skia] --> B[Port Plan in docs/design_docs]
    B --> C[Bazel workspace + package targets]
    C --> D[Incremental C++ modules under src/include]
    D --> E[Target-by-target parity checkpoints]
```

## Security / Privacy
- Inputs are repository-local source files and compiler/runtime dependencies.
- Trust boundary is local workspace state only; no user-provided binary assets are executed during bootstrap.
- Repository hygiene: lock external source fetch to explicit commands and commit `third_party` vendor location.

## Testing and Validation
- No functional test execution is required in this bootstrap milestone.
- Validation of bootstrap includes:
  - Confirm `WORKSPACE.bazel` and package BUILD files are syntactically present.
  - Confirm `third_party/tiny-skia` is checked in locally.
  - Confirm design doc gate remains updated before implementation work continues.
