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
- Use lowerCamelCase for all function names (e.g., `libraryVersion`, `catchOverflow`).
- Prefer deterministic, bit-accurate implementations and explicit comments only when non-obvious.
- Keep edits minimal and consistent with existing style in touched files.
- Run `bazel build //...` after each implementation step.
- Add/extend C++ tests as each file is ported and validate with `bazel test //...`.

## LLM-Specific Guidance

- Use `AGENTS.md` and `docs/design_docs/AGENTS.md` as the source of truth for local instruction.
- Keep design and implementation steps actionable and testable.
- Before taking risky actions (large refactors, deletions, destructive git operations), confirm intent.
- Do not commit any changes without explicit user approval in the current session.
- Treat the exact user phrase “Commit and next step” as the hard commit handoff:
  1) commit all currently outstanding working-directory changes with `git add -A`,
  2) then continue to the next requested task only after receiving a new user instruction.
- The phrase “Next step” by itself is non-committal; it does not authorize a commit.
- After a “Commit and next step” handoff, do not commit subsequent new work without
  another explicit commit approval from the user.
- “All currently outstanding working-directory changes” means every modified, new, and
  deleted file.
- “All currently outstanding working-directory changes” means every modified, new, and
  deleted file.

## Building

- Favor Bazel targets and keep Bazel files organized by module.
- Keep build configuration explicit and easy to diff for review.
- Avoid adding unnecessary dependencies.

## Docs

- Prefer concise Markdown with the repository-specific section structure.
- Keep markdown paths and links simple and stable.
