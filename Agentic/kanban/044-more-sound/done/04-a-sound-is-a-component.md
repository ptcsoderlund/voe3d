# 04 — A sound is a component a thing carries
folder: audio
after: 03
decisions: 0168, 0304

## Change
0304 point 6. Read `audio/audio.md`, `audio/include/audio/mixer.h` (the handle),
`3d/include/3d/emitter_component.h`, `3d/src/emitter_component.c` and
`3d/tests/emitter_component.c` — the pattern this card copies: a described row, its replace
intent and a control, a runtime-only row, register, add, get, the table and the submits.

- New `audio/include/audio/sound_component.h`:
  - `VOE_AUDIO_SOUND_PATH` 128.
  - `VOE_AUDIO_SOUND_FIELDS`: `path` CHAR of that size, `playing` BOOL, `loop` BOOL, `volume`
    FLOAT32, `pitch` FLOAT32; `voe_audio_sound` described from it; `voe_audio_sound_key`.
  - `voe_audio_sound_intent` {entity, sound}: the replace.
  - `voe_audio_sound_control_kind`: `VOE_AUDIO_SOUND_PLAY`, `_STOP`, `_TUNE`;
    `voe_audio_sound_control` {entity, kind, volume, pitch}; `voe_audio_sound_control_key`.
  - `voe_audio_sound_voice` {`voe_audio_voice voice`, `bool restart`}, runtime-only, never
    saved; `voe_audio_sound_voice_key`.
  - `voe_audio_sound_register(world, capacity)`: default row (empty path, playing, no loop,
    volume 1, pitch 1), menu "Audio / Sound", the intent as its replace, no need, the control
    and the runtime row, all with room for `capacity`.
  - `voe_audio_sound_add`, `_get`, `_count`, `_rows`, `_entities`, `voe_audio_sound_voice_get`,
    `_count`, `_rows`, `_entities`, `voe_audio_sound_submit`, `voe_audio_sound_control_submit`,
    each as the emitter's.
  - Header points: what a sound is (0304 point 6); placed at its thing's world position when it
    has a transform and followed, else unplaced; the default plays at once; play restarts from
    the start, stop ends it, tune sets volume and pitch; state changes only in the sound
    system (card 05); the runtime row holds the voice.
- New `audio/src/sound_component.c`: the keys, registration, and the read calls.
- New `audio/tests/sound_component.c`: registered on a world with transforms, the default row
  is as above with the menu path; add, get and the table agree; a replace and a control submit
  are accepted; the runtime row is registered runtime-only.
- `audio/audio.md`, `audio/include/audio/audio.md`, `audio/src/src.md`, `audio/tests/tests.md`:
  their entries.

## Done when
`cmake --build --preset debug --target voe_test_audio_sound_component && ctest --test-dir build/debug -R '^audio/sound_component$'`
passes.
