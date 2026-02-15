# AGENTS Instructions for `tiny-skia-cpp`

This repository is a C++20, Bazel-first porting effort for `tiny-skia` (Rust) to C++.

## Scope and Process

- Start every feature by writing a design doc under `docs/design_docs`.
- Keep design docs in the `docs/design_docs` directory and follow the templates there.
- Use `docs/design_docs/AGENTS.md` as the authoritative doc-style guide.

## Coding Style

- Use C++20 and Bazel as the primary build system.
- Keep line length under 100 characters when practical.
- Use clear naming, strong types, and explicit ownership boundaries.
- Use lowerCamelCase for all function names (e.g., `catchOverflow`, `setForceHqPipeline`).
- Prefer deterministic, bit-accurate implementations and explicit comments only when non-obvious.
- Keep edits minimal and consistent with existing style in touched files.
- Run both `bazel build //...` and `bazel test //...` after each implementation step.
- Add/extend C++ tests as each file is ported before running the per-step build/test gate.
- Prefer Google Mock matchers (`EXPECT_THAT`, `ASSERT_THAT`) for container/value assertions
  when they provide better diagnostics than plain equality checks.
- Prioritize diagnosable failures: choose assertion styles that clearly report element-level
  mismatches (for example `ElementsAre`/`Pointwise`) over opaque pass/fail comparisons.
- It is explicitly acceptable to add custom matchers when they improve readability and
  diagnostics (for example `ScreenIntRectEq`).
- Prefer reusable matcher helpers in `src/**/tests/test_utils/` (header-only where practical)
  and share them across tests instead of duplicating ad-hoc matcher logic per file.
- Keep tests colocated with source modules:
  - `src/tiny_skia/Foo.cpp` -> `src/tiny_skia/tests/FooTest.cpp`
  - `src/tiny_skia/subdir/Bar.cpp` -> `src/tiny_skia/subdir/tests/BarTest.cpp`
- Update the design doc immediately when a function or module is marked complete or in progress.
- Keep milestone checkboxes synchronized with code changes (any new/edited/deleted file must
  be reflected by an accurate status change in the design tracker).

## LLM-Specific Guidance

- Use `AGENTS.md` and `docs/design_docs/AGENTS.md` as the source of truth for local instruction.
- Keep design and implementation steps actionable and testable.
- For any code change (new/edited/deleted file), update milestone checkboxes or function
  status entries in the same update batch as implementation.
- During porting, explicitly report good sequence points for incremental commits.
- Explicitly call out when a full Rust source file mapping is complete in C++ so the user can
  decide to commit in pieces.
- Before taking risky actions (large refactors, deletions, destructive git operations), confirm intent.
- No commit is allowed without explicit user approval in this session, reviewed live.
- Explicit user approval is required for **every** commit operation (including after previous
  handoff phrases), regardless of any shorthand wording.
- There is no implied commit permission. Always ask before running `git commit`.
- The user phrase “Commit and next step” is only valid when it appears as an explicit request and
  indicates approval for the current outstanding working-directory diff only.
- “Next step” and similar non-commit phrases are explicitly non-committal and must not trigger any
  commit.
- Never commit partially. If a commit is approved, include all currently outstanding working-directory
  changes in that commit.

## Building

- Favor Bazel targets and keep Bazel files organized by module.
- Keep build configuration explicit and easy to diff for review.
- Avoid adding unnecessary dependencies.

## Docs

- Prefer concise Markdown with the repository-specific section structure.
- Keep markdown paths and links simple and stable.

## Learned Patterns and Recommendations

- For `std::optional` results in tests, prefer matcher assertions over `has_value()` checks:
  - present: `ASSERT_THAT(value, Optional(_))`
  - absent: `EXPECT_THAT(value, Eq(std::nullopt))`
- When asserting multiple related scalar results, prefer aggregating into arrays/spans and using
  `ElementsAre(...)` (or `UnorderedElementsAre(...)`) to improve mismatch diagnostics.
- Promote repeated domain assertions into reusable matchers under
  `src/**/tests/test_utils/` instead of copy/pasting local helper loops.
- Prefer `Field(...)`, `Property(...)`, and `AllOf(...)` for struct/member diagnostics when
  failures should identify exactly which field mismatched.
- Keep matcher upgrades behavior-preserving: no production logic changes in readability-only
  batches.
- Diminishing returns rule: once remaining work is mostly isolated scalar equality checks with
  clear failure output already, prioritize new porting work over further matcher refactors.
- In readability-only test passes, keep edits scoped and still run the full gate:
  `bazel build //...` and `bazel test //...`.
