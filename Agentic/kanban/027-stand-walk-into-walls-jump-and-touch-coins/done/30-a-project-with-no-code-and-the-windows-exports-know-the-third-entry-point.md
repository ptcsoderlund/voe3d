# 30 — A project with no code, and the Windows exports, know the third entry point
folder: cmake
decisions: 0168, 0257, 0245

## Change
Card 29 declared `voe_game_project_systems_after_move` in `game/project.h`; the build files that
name the project's entry points learn it.

- `cmake/game.cmake` — the `no_code.c` written for a project with no code defines
  `voe_game_project_systems_after_move` empty, like the other two; its first comment line says
  empty entry points still.
- `cmake/exports.cmake` — the header paragraph on skipped members and the first-pass regex name
  all three entry points.
- `cmake/cmake.md` — add the missing entry for `exports.cmake` (the `.def` the editor links on
  Windows, skipping the members that call a project's entry points).

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder cmake` prints `FINDINGS: 0`.
2. `grep -c voe_game_project_systems_after_move cmake/game.cmake cmake/exports.cmake` shows at
   least 1 for each file.
