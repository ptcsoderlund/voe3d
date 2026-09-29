# 04 — The game opens the window it is handed
folder: game
after: 03
decisions: 0168, 0291, 0234, 0237

## Change
0291 point 3. Files: `game/include/game/run.h`, `game/src/run.c`, `game/include/game/game.md`,
`game/src/src.md`. The generated `main.c` that calls it is card 06's, in `editor`; do not touch
it.

- `run.h`: `VOE_GAME_WIDTH` and `VOE_GAME_HEIGHT` go. A `voe_game_window` struct: `int width`,
  `int height`, `bool fullscreen`. `int voe_game_run(const char *title, voe_game_window
  window);` Header points: the example `main` passes a window; the window comes from the
  project's settings through the generated `main.c`, so the game reads no project text (0236);
  fullscreen takes the screen; the window can be resized to any shape and the world is drawn at
  the window's aspect, never stretched; the "fixed until a project setting" constraint goes.
- `run.c`: the app's settings take width, height and fullscreen from `window`; assert a width and
  height above nought.
- `game.md` and `src.md`: the `run.h` and `run.c` entries lose "at 1280×720" and say the handed
  window.

## Done when
`cmake --build --preset debug --target voe_game` exits 0, `ctest --test-dir build/debug -R
'^game/'` passes, and `! grep -rn VOE_GAME_WIDTH game` exits 0.
