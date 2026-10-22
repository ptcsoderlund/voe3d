# 09 — A game says its counts when asked
folder: game
after: 06
decisions: 0168, 0464, 0465

## Change
0465 point 2 in Play and the shipped game, and 0464 point 4's order in the game's frame.

- `game/src/frame.c` — in `voe_game_frame`: `voe_3d_draw_system_light_blockers`,
  `voe_3d_draw_system_point_lights` and `voe_3d_draw_system_lights` move to just after the
  `foliage` arena mark, then `voe_3d_draw_system_foliage`, then the `mark`, then the shadows, so
  the gather knows every sun and the groups and lights outlive the pass. The counts
  `voe_3d_draw_system_run` returns are written to the new out parameter.
- `game/include/game/frame.h` — `voe_game_frame` gains a last `voe_3d_view_counts *counts`, NULL
  for none, written only when the world is drawn; its comment says so and that the gather follows
  the lights.
- `game/src/run.c` — at start, once, `voe_platform_environment_read("VOE_GAME_COUNTS", …)`;
  when it is `1` the loop hands `voe_game_frame` a counts record and, once a second of the
  frame's elapsed time, writes `game: view: drawn D, skipped S; shadows: drawn C, skipped K` on
  stderr; otherwise it hands NULL and writes nothing. The file header gains a line naming the
  variable (0465).
- `game/tests/frame.c` — its calls pass NULL; one case passes a record on a world with a shape in
  front of the camera and one behind and checks drawn 1, skipped 1.
- `game/src/src.md`, `game/include/game/game.md` — the `frame.*` and `run.c` entries follow.

## Done when
Test `game/frame` passes, and `grep -n "VOE_GAME_COUNTS" game/src/run.c` prints a line.

Human (How to test 6): with `VOE_GAME_COUNTS=1` in the editor's environment, press Play on the
hill: stderr shows the line once a second, drawn falling and skipped rising as the view turns to
the sky, and the picture looks as before. Ship the project and run the game with the same
variable: the same.
