# 88 — An off camera is heard from the origin
folder: audio
after: none
decisions: 0168, 0427, 0428, 0441

## Change
0441 point 5: sound is heard from where the view now stands. With the
camera off, the frame looks from the origin (card 86); the listener does
the same, so what is heard on the left is what is seen on the left.

`audio/src/sound_system.c` (243 lines; open its header comment and the
listener function, about lines 151-180): when the camera found is on an
entity `voe_scene_identity_off` (`scene/identity_component.h`) says is
off, the listener is made from the identity pose (position nought, +X
right, -Z forward) and the camera table's default row's `fov_y`
(`voe_ecs_component_default` with
`voe_ecs_component_type(world, &voe_scene_camera_key)`,
`ecs/component.h`); otherwise as today. The function's comment and the
file's header say so (0441).

`audio/include/audio/sound_system.h`: in A RUN, IN ORDER, the listener
point gains that an off camera (itself or through a parent) is heard as a
default camera at the origin looking down -Z (0441). Signature unchanged.

`audio/tests/sound_system.c` (256 lines; read its lines 69-130 for
`a_world`, `switched` and `a_camera`): `a_camera` takes the position (the
existing callers pass the origin) and returns the entity, given an
identity as `switched` needs; register the parent table only if a case
needs it. New case `an_off_camera_hears_from_the_origin`: a camera at
(6, 0, 0) unturned and a loop at (3, 0, -3): after a run the mix is louder
left; the camera's entity off and a run: louder right, as from the origin;
on and a run: louder left again. In `main` and the file's header;
`audio/tests/tests.md`'s `sound_system.c` entry gains "an off camera heard
from the origin", under 300 characters.

`audio/src/src.md`: the `sound_system.c` entry, if it describes the
listener, names the off camera; one sentence under 300 characters.

## Done when
The test `audio/sound_system` passes after the folder's build with the new
case.
