# 02 — A voice has a handle, loops, and is tuned while it plays
folder: audio
after: none
decisions: 0168, 0265, 0266, 0304

## Change
0304 points 2 to 4, with no placement yet (card 03). Read `audio/audio.md`,
`audio/include/audio/mixer.h`, `audio/src/mixer.c`, `audio/src/src.md`, `audio/tests/mixer.c`,
`audio/tests/tests.md`.

`audio/include/audio/mixer.h`:
- `VOE_AUDIO_VOICES` 32.
- `typedef struct { uint32_t id; } voe_audio_voice;` id 0 is none; slot and generation.
- `typedef struct { const char *path; bool loop; bool held; float volume; float pitch; } voe_audio_start;`
  (card 03 adds the place).
- `voe_audio_voice voe_audio_mixer_start(voe_audio_mixer *, voe_audio_start)`: 0 for `""`, a
  failed clip, or every voice looping.
- `voe_audio_mixer_play` keeps its signature: a start with no loop, not held, volume 1, pitch 1.
- `void voe_audio_mixer_stop(voe_audio_mixer *, voe_audio_voice)`,
  `bool voe_audio_mixer_playing(const voe_audio_mixer *, voe_audio_voice)`,
  `void voe_audio_mixer_tune(voe_audio_mixer *, voe_audio_voice, float volume, float pitch)`,
  `void voe_audio_mixer_sweep(voe_audio_mixer *)`. A stale handle is a no-op, false for
  `_playing`.
- Header points to add: a handle and why stale is harmless; a loop has no seam; pitch is read
  rate, clamped; volume clamped; a gain change ramps over the next mix; the steal rule (oldest
  that does not loop; all looping refuses); held voices and the sweep. The existing
  "A PLAY OVERLAPS" point is amended for the new steal rule.

`audio/src/mixer.c`: each voice gets a generation, a fractional read position, loop, held,
touched-since-sweep, a target gain and the gain it is ramping from; the mix reads with linear
interpolation at `pitch`, wraps a loop across its end, ramps gain linearly across the call's
frames. `_tune` marks the voice touched; `_sweep` stops every held voice not touched since the
last sweep and clears the marks. If the file passes ~350 lines, move the voice read-and-sum
into `audio/src/voice.c` with its own header comment and a private `audio/src/voice.h`.

`audio/tests/mixer.c`, new cases: a loop of a short ramp mixed past its end continues from its
first frame with no zero between; pitch 2 plays a clip in half the frames; volume 0.5 halves a
steady sample after the ramp; `_stop` silences at the next mix and a stale handle after it is
a no-op; 32 loops started, a 33rd start returns 0, and a one-shot then takes the oldest
non-loop; a held voice not tuned stops at the second sweep while a tuned one plays on.

`audio/audio.md`, `audio/include/audio/audio.md`, `audio/src/src.md`, `audio/tests/tests.md`:
the entries that name what the mixer does and what the test covers.

## Done when
`cmake --build --preset debug --target voe_test_audio_mixer && ctest --test-dir build/debug -R '^audio/mixer$'`
passes.
