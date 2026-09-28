# 28 — The editor's model store copies its folder without strdup
folder: editor/src
decisions: 0168

## Change
Bug 03: the Windows build (clang with the MSVC runtime, warnings as errors) stops at
`editor/src/models.c` line 62, where `voe_editor_models` keeps its own copy of the folder with
`strdup`; the UCRT marks that name deprecated. It is the tree's only `strdup`. The tree's other
string copies (`3d/src/models.c`, `editor/src/settings.c`) use `strlen` + `malloc` + `memcpy`;
do the same here.

In `editor/src/models.c` only: replace the `strdup(folder)` call with a copy of `strlen(folder)
+ 1` bytes into a `malloc`ed buffer, keeping the out-of-memory handling the line already has
(or asserting as `voe_editor_models_new` does if it has none). No new function, no new include
beyond what the file already has, no build-flag change. Change no other file.

## Done when
`! grep -rnw --include='*.c' --include='*.h' strdup base platform render 3d audio physics game
editor scene 2>/dev/null` exits 0 (no `strdup` left in the tree's C sources), and
`bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0 and prints `FINDINGS: 0`.

For the human: on Windows, build `voe_editor` in CLion (debug preset, clang); it builds past
`editor/src/models.c`.

## Blocked
The change is made and `checks.sh --folder editor/src` prints `FINDINGS: 0`; the grep for `strdup` passes. `checks.sh --all` still exits 1 on one finding outside this folder: `Agentic/tech-lead/system.md` is over 60 lines at 100 columns (it was already there before this change, last touched in bb3b21f). Trimming `system.md` (or raising `--system-cap`) is tech-lead work; once that is done, `--all` should pass and this card can move to `done/`.
