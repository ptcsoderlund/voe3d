# 03 — A voice is placed from a listener
folder: audio
after: 01, 02
decisions: 0168, 0304

## Change
0304 point 5. Read `audio/audio.md`, `audio/include/audio/mixer.h`, `audio/src/mixer.c` (and
`audio/src/voice.c`/`voice.h` if card 02 made them), `audio/CMakeLists.txt`,
`math/include/math/double3.h`, `math/include/math/float3.h`, `audio/tests/tests.md`.

- `audio/CMakeLists.txt`: `voe_module(audio DEPENDS scene ecs math platform assets base)`.
- New `audio/include/audio/place.h` and `audio/src/place.c`:
  - `VOE_AUDIO_REFERENCE` 10.0f metres.
  - `voe_audio_listener`: `voe_math_double3 position`, `voe_math_float3 right`,
    `voe_math_float3 forward`, `float tan_half_width`, `float reference`.
  - `voe_audio_listener voe_audio_listener_make(voe_math_double3 position, voe_math_float3 right, voe_math_float3 forward, float tan_half_width)`:
    reference is the distance along `forward` to the plane y = 0 when it lies ahead, else
    `VOE_AUDIO_REFERENCE`.
  - `typedef struct { float left; float right; } voe_audio_gains;`
  - `voe_audio_gains voe_audio_place(const voe_audio_listener *, voe_math_double3 where)`:
    falloff times the balance law, exactly as 0304 point 5 gives; a point at the listener is
    full and centred. `voe_audio_gains voe_audio_unplaced(void)` is 1 and 1.
  - Header points: why the reference comes from the view axis (a high top-down camera), why
    pan is screen x and not angle, why the balance law (the coin keeps its loudness), the
    difference taken in double and turned to float, as the render view does (0250).
- `audio/include/audio/mixer.h` and the mixer source:
  - `voe_audio_start` gains `bool placed; voe_math_double3 where;`.
  - `void voe_audio_mixer_listen(voe_audio_mixer *, const voe_audio_listener *)`: copied; NULL
    is none, and then placed voices play unplaced.
  - `void voe_audio_mixer_move(voe_audio_mixer *, voe_audio_voice, voe_math_double3 where)`:
    places the voice and marks it touched for the sweep; stale is a no-op.
  - `voe_audio_voice voe_audio_mixer_play_at(voe_audio_mixer *, const char *path, voe_math_double3 where)`:
    a placed one-shot.
  - Each mix takes every voice's left and right target from the listener and its place, and
    ramps to them across the call as gain already does; the volume multiplies both.
  - Header points: a voice may be placed, by what law (point to `place.h`), and follows a
    `_move`.
- New `audio/tests/place.c`: a point straight ahead is centred and full at the reference; one
  at the right screen edge pans +1, the left edge -1; one behind on the left pans -1; twice the
  reference is a quarter; a camera 20 m up looking straight down has reference 20, one looking
  level has `VOE_AUDIO_REFERENCE`.
- `audio/tests/mixer.c`, new case: a steady clip played at a point left of a listener mixes
  louder in the left channel than the right; with the listener cleared the two are equal.
- `audio/audio.md`, `audio/include/audio/audio.md`, `audio/src/src.md`, `audio/tests/tests.md`:
  entries for `place.h`, `place.c`, the test, and the mixer's placement.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_test_audio_place voe_test_audio_mixer && ctest --test-dir build/debug -R '^audio/(place|mixer)$'`
passes.
