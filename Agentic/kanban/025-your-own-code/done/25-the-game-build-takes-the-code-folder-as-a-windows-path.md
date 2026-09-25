# 25 — The game build takes the project's code folder as a Windows path
folder: game
decisions: 0168, 0242, 0245

## Change
Bug 03. The editor writes `VOE_PROJECT_CODE` as the project's native path, on Windows with
backslashes. The configure passes, but CMake copies the `CONFIGURE_DEPENDS` glob expression
unescaped into `CMakeFiles/VerifyGlobs.cmake`, and the next build fails to parse `\U`. The fix
goes once, where the variable is read, so any tree including `game.cmake` is covered and the
editor's writer is left alone. Read the header of `cmake/game.cmake` (a build file `--folder`
allows beside a card naming a decision).

- `cmake/game.cmake` — before the `file(GLOB voe_project_sources ...)` block, when
  `VOE_PROJECT_CODE` is set, rewrite it in CMake's own path form with
  `cmake_path(SET VOE_PROJECT_CODE NORMALIZE "${VOE_PROJECT_CODE}")`. It turns backslashes into
  `/` on Windows and leaves a Linux path as it is. Every later use (the glob, the include path) reads
  the rewritten value. The header's `VOE_PROJECT_CODE` paragraph gets two points: the variable
  may be a native path, and it is turned into CMake's form first, because a glob with
  `CONFIGURE_DEPENDS` is copied into VerifyGlobs.cmake unescaped, where a backslash is an escape
  (bug 03).
- `game/game.md` — only if an entry describes how `VOE_PROJECT_CODE` is read.

## Done when
1. `checks.sh --folder game` prints `FINDINGS: 0`.
2. `grep -n 'cmake_path(SET VOE_PROJECT_CODE' cmake/game.cmake` prints a line numbered before the
   line from `grep -n 'file(GLOB voe_project_sources' cmake/game.cmake`.
3. In `p=$(mktemp -d)` with `cp -r examples/capsule/. $p`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty
   and `$p/Build/editor/loaded/project-1.so` exists.
4. `checks.sh --all` prints `FINDINGS: 0`.
5. The human's, on Windows: build the editor, open `examples/capsule/` and build its code. The
   build log has no `VerifyGlobs` error, the code loads, and `## How to test` in `feature.md`
   behaves as on Linux (bug 03).
