# 06 — The game's world holds sounds and plays them each step
folder: game
after: 05
decisions: 0168, 0304

## Change
0304 point 7. Read `audio/include/audio/sound_component.h`,
`audio/include/audio/sound_system.h`, `platform/include/platform/window.h` (`_size`), and in
`game/`: `game.md`, `include/game/game.md`, `include/game/world.h`, `src/world.c`,
`include/game/scene.h`, `include/game/frame.h`, `src/frame.c`, `include/game/steps.h`,
`src/steps.c`, `src/src.md`, `tests/world.c`, `tests/steps.c`, `tests/tests.md`.

- `include/game/world.h`: `VOE_GAME_WORLD_SOUNDS`, defined as `VOE_GAME_WORLD_MAX_DRAWN`, with
  its reason (a sound sits on a thing, spawned ones too); `VOE_GAME_WORLD_TYPES` 19; the
  header's list names the sound and its voice row, and "seventeen" becomes "nineteen".
- `src/world.c`: `voe_audio_sound_register(world, VOE_GAME_WORLD_SOUNDS)` after the emitter's.
- `include/game/scene.h`: includes `audio/sound_component.h`.
- `src/frame.c`, `voe_game_world_step`: `voe_audio_sound_system_run(world, NULL, 1.0f)` last,
  so replaces and controls are applied wherever the world steps, the editor included, and
  nothing plays there. `include/game/frame.h`: the order names it.
- `src/steps.c`: after the emitter system, when `audio` is not NULL,
  `voe_audio_sound_system_run(world, audio, aspect)`, the aspect the window's width over
  height, 1 when the window is NULL or has no height. `include/game/steps.h`: the step's order
  and its constraints name it.
- `tests/world.c`: the type count, and the sound's and its voice row's keys resolve.
- `tests/steps.c`: a new case: a looping sound on a thing with a transform, a WAV the test
  writes in its working directory, a mixer on that folder, two steps through
  `voe_game_steps_run`: the thing's voice row holds a voice `voe_audio_mixer_playing` answers
  true for.
- `game.md`, `include/game/game.md`, `src/src.md`, `tests/tests.md`: the entries naming the
  type count, the world step's order and the step's.

## Done when
`cmake --build --preset debug --target voe_game voe_test_game_world voe_test_game_steps voe_test_game_frame && ctest --test-dir build/debug -R '^game/(world|steps|frame)$'`
passes.
