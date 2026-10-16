# 25 — A sound that is off is silent
folder: audio
after: 19
decisions: 0168, 0427, 0428

## Change
0428 point 6.

`audio/src/sound_system.c`, `reconcile`: when
`voe_scene_identity_off(world, entity)` (`scene/identity_component.h`) is
true the voice is stopped and the voice row zeroed, as for a sound not
`playing`, but the sound row's `playing` is not written. Switched on again,
a playing sound has no voice and so is started by the existing branch. A
one-shot that ended while off is not ended by the switch: the "ended"
test reads a live voice, and an off sound has none. The function's comment
says why `playing` is kept. The file's header gains "an off sound is
stopped, not ended".

`audio/include/audio/sound_system.h`: the run's comment, where it says
what a row's reconcile does, gains that an off entity's sound is silent and
resumes from its start when switched on.

`audio/tests/sound_system.c`: its world also registers the identity table
(`scene/identity_system.h`), the sound's entity given an identity. New
case `an_off_loop_is_silent_and_comes_back`: a loop playing as the existing
loop case sets it; its identity off and a run: the mixer has no live voice
for it and the row still says `playing`; on and a run: a live voice again.
In `main` and the header; `audio/tests/tests.md`'s `sound_system.c` entry
gains "a loop switched off and back", under 300 characters.

## Done when
The test `audio/sound_system` passes after the folder's build with the new
case.
