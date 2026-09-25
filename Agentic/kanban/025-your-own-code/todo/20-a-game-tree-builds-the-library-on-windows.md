# 20 — A game tree builds the project's library on Windows
folder: game
decisions: 0168, 0175, 0242, 0243, 0245

## Change
Points 2–4 of 0245, `game.cmake`'s half, and the end of bug 01. Read the header of
`cmake/game.cmake` (a build file `--folder` allows beside a card naming a decision).

- `cmake/game.cmake` — the `WIN32` refusal of `VOE_GAME_LIBRARY` goes. In library mode on
  `WIN32`: an unset or missing `VOE_EDITOR_IMPORTS` is a `FATAL_ERROR` naming it; `project`
  links `VOE_EDITOR_IMPORTS`, compiles with the definition `VOE_BASE_IMPORTING`, has
  `WINDOWS_EXPORT_ALL_SYMBOLS ON`, and `RUNTIME_OUTPUT_DIRECTORY` (where a DLL goes) is
  `CMAKE_BINARY_DIR` beside the existing `LIBRARY_OUTPUT_DIRECTORY`. Linux is unchanged. The
  header: the ON mode names `project.dll` on Windows, the import library it links, the macro
  and why each (0245); the Constraints line about Linux-only goes.
- `game/game.md` — only if an entry says library mode is Linux-only.

## Done when
1. `checks.sh --folder game` prints `FINDINGS: 0`.
2. `grep -c 'Linux-only' cmake/game.cmake` prints 0.
3. In `p=$(mktemp -d)` with `cp -r game/example/. $p`: the capture of card 19's check 3 exits 0
   with `$p/err` empty and `$p/Build/editor/loaded/project-1.so` present.
4. `checks.sh --all` prints `FINDINGS: 0`.
5. The human's, on Windows: build and start the editor, then walk `## How to test` in
   `feature.md` with the example project. Every step behaves as on Linux (bug 01).
