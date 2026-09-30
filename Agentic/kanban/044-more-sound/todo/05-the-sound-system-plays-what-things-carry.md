# 05 — The sound system plays what things carry
folder: audio
after: 04
decisions: 0168, 0304

## Change
0304 point 7. Read `audio/audio.md`, `audio/include/audio/mixer.h`,
`audio/include/audio/place.h`, `audio/include/audio/sound_component.h`,
`3d/include/3d/emitter_system.h` (the shape of a system header),
`scene/include/scene/camera_component.h`, `scene/include/scene/transform_component.h`,
`ecs/include/ecs/world.h`, `ecs/include/ecs/structure.h`, `audio/tests/mixer.c` (how a test writes its WAVs).

- New `audio/include/audio/sound_system.h`:
  `void voe_audio_sound_system_run(voe_ecs_world *world, voe_audio_mixer *mixer, float aspect)`.
  Header points, as a run in order: drains replaces (dead or sound-less entity dropped; a path
  with no NUL ended and reported once a run, as the emitter's texture) and controls (play sets
  playing and restart, stop clears playing, tune sets volume and pitch); adds a runtime row to
  each sound lacking one, drops rows whose sound is gone; then, only with a mixer: the listener
  from the first camera whose entity has a transform (world position, its local +X and -Z
  turned by the world rotation, `tan(fov_y / 2) * aspect`, through `voe_audio_listener_make`),
  none without one; each row: restart or playing without a live voice starts one (held, loop,
  volume, pitch, placed at the world position when it has a transform), not playing stops
  it, else tune and move; a one-shot whose voice ended clears `playing`; then the sweep. NULL
  mixer: drains and adds only, which is the editor's case. Asserts `aspect > 0`.
- New `audio/src/sound_system.c`: that run.
- New `audio/tests/sound_system.c`, with WAVs it writes as the mixer test does: a looping sound
  on a thing with a transform plays after a run; a camera at the origin looking down -Z and the
  thing at +X in front mixes louder right than left; a stop control silences it; a one-shot
  run past its end has `playing` false; the thing removed through the structural queue
  (`voe_ecs_structure_apply`) and one more run leaves its voice not playing; a run with NULL
  mixer applies a replace and starts nothing.
- `audio/audio.md`, `audio/include/audio/audio.md`, `audio/src/src.md`, `audio/tests/tests.md`:
  their entries.

## Done when
`cmake --build --preset debug --target voe_test_audio_sound_system && ctest --test-dir build/debug -R '^audio/sound_system$'`
passes.
