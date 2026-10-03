# 02 — A game's interface pauses the run and starts the level again
folder: game
after: 01
decisions: 0168, 0259, 0333

## Change
0333 points 1–4. Read the files below, and the header of `audio/include/audio/mixer.h` for
`voe_audio_mixer_pause`.

- `game/include/game/project.h`: `typedef struct { bool paused; bool restart; }
  voe_game_project_asks;` and `voe_game_project_asks *asks` last in `voe_game_project_frame`,
  never NULL. A header paragraph: the interface may pause the run and start the level again,
  what each does, both read after the interface each frame (0333).
- `game/include/game/interface.h`, `game/src/interface.c`: `voe_game_interface_run` takes
  `voe_game_project_asks *asks` after `size`, asserts it is not NULL and puts it in the frame.
  The usage block to match.
- `game/src/run.c`: the run keeps one asks, both false. The world moves into an arena of its
  own (the interface keeps `arena`). Each frame, before the steps, a set `restart`: that arena
  cleared, `voe_game_world_new`, `voe_game_project_register`, `voe_game_scene_build` (a refusal
  is the start's refusal and line), the steps' bank zeroed, `voe_game_models_update`; then both
  asks false. While `paused`, `voe_game_steps_run` is not called and the draw keeps the last lag.
  After the interface, `voe_audio_mixer_pause(mixer, asks.paused)`. Factor the world's making
  into one static function used at the start and at a restart.
- `game/include/game/run.h`: THE ORDER paragraph gains the restart and the pause; the interface
  paragraph says it may pause and restart.
- `game/tests/interface.c`: a stand-in project that sets both asks; after the run the caller's
  asks read true. `game/tests/frame.c`: its interface run passes an asks.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the project, interface,
  run and interface-test entries say the asks, each a phrase.

## Done when
`ctest --test-dir build/debug -R "^game/"` passes.
