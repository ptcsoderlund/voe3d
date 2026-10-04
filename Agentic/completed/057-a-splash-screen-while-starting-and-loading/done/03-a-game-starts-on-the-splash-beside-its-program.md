# 03 — A game starts on the splash beside its program
folder: game
after: 02
decisions: 0168, 0263, 0346, 0356

## Change
The engine's copy of the splash comes into `game`, and the game's run shows the splash it finds
beside its program.

- `game/src/splashscreen.png` (new): `cp "engine_assets/Engine images/splashscreen.png"
  game/src/splashscreen.png`, unchanged; the original stays (0263). Listed on
  `game/src/src.md` as the engine's splash, read by the editor from the source and put beside a
  game's program by `cmake/game.cmake` when the project has none (0356).
- `game/src/run.c`: right before the starting prepare, read `splashscreen.png` in the
  program's folder (the folder the run already joins sounds and models onto) with
  `voe_app_picture_read` (app/picture.h) into the scratch arena; on success pass it to
  `voe_game_starting_prepare`, else one stderr line naming the path and pass NULL. Rewind the
  scratch after the prepare as now, and give the texture back with
  `voe_render_texture_destroy` once the prepare is done (no game changes scene yet, 0356).
- `game/include/game/run.h`: THE ORDER paragraph says the splash is read beside the program
  before the starting line, shown with it, the plain screen when it cannot be read, and given
  back after.

## Done when
`cmp game/src/splashscreen.png "engine_assets/Engine images/splashscreen.png"` exits 0, and
`cmake --build --preset debug --target voe_game` succeeds.
