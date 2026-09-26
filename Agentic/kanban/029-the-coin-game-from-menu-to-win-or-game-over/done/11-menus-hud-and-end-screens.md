# 11 — Menus, HUD and end screens
folder: examples/coin_game
decisions: 0168, 0259, 0260, 0194
read: feature.md

## Change
Read `examples/coin_game/Code/game_state.h`, `Code/game_system.c`, `Code/project.c`,
`game/include/game/project.h`, `ui/include/ui/widgets.h`, `ui/include/ui/layout.h`,
`platform/include/platform/input.h` (keys).

- `Code/game_state.h`, `Code/game_interface.c` (new) — `bool game_interface_run(const
  voe_game_project_frame *frame)`: no row yet, ends the frame and returns true. Else one root
  over `frame->size`:
  - menu: a centred panel, Start and Quit;
  - playing: a HUD panel at the top left, the score and the coins left;
  - won: "You win", the final score, Restart and Quit; lost: "Game over", Restart and Quit.
  Ends the frame, then: Enter's edge presses the first button, Escape's the last (levels kept
  in the row). Start sets playing; Restart sets playing, restarts + 1, dropped and banked 0;
  Quit returns false. Header: the screens and their keys; this module still the row's one
  writer.
- `Code/project.c` — the interface entry point calls it.
- `Code/Code.md` — the entry.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: every step of `## How to test` in `feature.md`, 1 to 12, in the editor on
   `examples/coin_game/`.
