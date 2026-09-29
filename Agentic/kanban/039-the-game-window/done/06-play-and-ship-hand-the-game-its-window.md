# 06 — Play and Ship hand the game its window
folder: editor
after: 04, 05
decisions: 0168, 0291, 0235, 0264

## Change
0291 point 3. Card 04 made `voe_game_run(const char *title, voe_game_window window)`
(`game/run.h`); card 05 put the window on `voe_editor_project`. Files:
`editor/src/game_tree.h`, `editor/src/game_tree.c`, `editor/src/src.md`.

- `game_tree.c`: the generated `main.c` calls `voe_game_run` with the title as now and a
  `(voe_game_window){ width, height, fullscreen }` literal from the project's window, `true` or
  `false` spelled out. It is still written only when its bytes differ, so a changed setting
  rebuilds the program and an unchanged one does not. Play and Ship both get it, since both
  write the same tree.
- `game_tree.h`: the layout paragraph says `main.c` carries the project's game window, and why
  it is numbers in the source and not a file the game reads (0236).
- `src.md`: the `game_tree.c` entry mentions the window in `main.c`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0 and
`grep -q voe_game_window editor/src/game_tree.c` exits 0. For the human: Play
`examples/tank_game`; `Build/game/main.c` passes 1280, 720 and `false`, and the game opens.
