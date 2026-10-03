# 01 — The mixer pauses and goes on where it held
folder: audio
after: none
decisions: 0168, 0333

## Change
0333 point 3. Read the files below and no others.

- `audio/include/audio/mixer.h`: `void voe_audio_mixer_pause(voe_audio_mixer *mixer, bool
  paused);`. A header paragraph: a paused mixer is silent and holds its place — a mix writes
  zeros and no voice advances, ramps or ends; start, stop, tune, move and sweep act as ever, a
  voice started while paused waiting at its first frame; unpaused, every voice goes on from the
  frame it held; the pump still writes, so the device never starves. The usage block gains the
  call.
- `audio/src/mixer.c`: the paused flag on the mixer, false at `_new`; `_mix` writes zeros and
  touches no voice while it is set.
- `audio/tests/mixer.c`: a looping voice mixed for a block, paused: the next block is all zeros
  and the voice still plays; unpaused: the block after equals the block an unpaused twin mixer
  gives after the same first block. A voice started while paused is silent until unpaused.
- `audio/tests/tests.md`: the mixer entry names the pause. `audio/audio.md`: the mixer entry
  names the pause.

## Done when
`ctest --test-dir build/debug -R "^audio/mixer$"` passes.
