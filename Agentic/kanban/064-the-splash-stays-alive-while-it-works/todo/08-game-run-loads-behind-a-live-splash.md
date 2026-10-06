# 08 — A game's start runs on the worker behind a live splash
folder: game
after: 07
decisions: 0168, 0362, 0370
read: feature.md

## Change
- `game/src/run.c`: the start after the interface and the splash's read becomes one
  `voe_game_starting_wait` (card 05) whose work, a static function over a context struct in this
  file, does in order: `voe_game_starting_shaders` with
  `voe_app_pipeline_cache_path(scratch, "pipelines_game.cache")`; the world in its own arena, the
  project's register and the scene built ("Loading scene"); the built-in shapes uploaded; the model
  store made and `voe_game_models_update` with the progress. The worker uses a scratch arena of its
  own, never the main thread's frame arena. The start log's steps for those parts are taken by the
  worker where their work ends; it is written on the main thread as now. After the wait: the
  splash's texture given back, then the mixer and sound device, then the loop, all as now. A closed
  window during the wait ends the run as a close does (0), releasing whatever the worker made; a
  failed step is 1 with its line, as now. The loop's two other `voe_game_models_update` calls
  pass NULL. The file header says which thread does what.
- `game/include/game/run.h`: THE ORDER describes the wait and what its worker does (0370 point 4).
- `game/include/game/starting.h`, `game/src/starting.c`: remove `voe_game_starting_prepare`, now
  unused here (the editor stops calling it in card 09; it breaks until then, do not touch it).
- `game/tests/starting.c`: drop the prepare loop's check.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries for starting and
  run.

## Done when
`cmake --build --preset debug --target voe_game` exits 0 and `ctest --test-dir build/debug -R
'^game/'` passes. Human, after card 09: How to test step 5 (ship the tank game, run it twice,
close it during its splash once).
