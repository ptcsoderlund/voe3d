# 86 — An off camera frames from the origin
folder: 3d
after: 85
decisions: 0168, 0427, 0428, 0441

## Change
Bug 11, its camera half: a switched-off camera still frames Play and a
shipped game. The game's frame, the editor's Play and corner picture and
dev all take the camera through `voe_3d_draw_system_frame`, so the fix is
there, once (0441 point 3).

`3d/src/draw_system.c` (682 lines; open only its header comment, lines
1-3, and `voe_3d_draw_system_frame` with the comment above it, about
lines 216-262):
- After the existing asserts, when `voe_scene_identity_off`
  (`scene/identity_component.h`) is true of the camera's entity, `lens` is
  the camera table's default row (`voe_ecs_component_default` with
  `voe_ecs_component_type(world, &voe_scene_camera_key)`, `ecs/component.h`;
  assert it is set) and `pose` is the identity transform: position nought,
  rotation w 1, scale 1 (`scene/transform_component.h` has no identity
  constant; build it in place). `lag` is not applied to it. Otherwise as
  today. The transform assert stays for an on camera only, or for both if
  simpler.
- The comment above the function names the off camera (0441).
- The file's header comment says "the camera, or a default one at the
  origin when it is off".

`3d/include/3d/draw_system.h` (818 lines, a public header: not split;
open only lines 438-492): `voe_3d_draw_system_frame`'s comment, in its
camera paragraph, says an off camera (itself or through a parent) frames
as a camera at the identity transform with the camera table's default row
(0441), the table still holding exactly one camera. Signature unchanged.

`3d/src/src.md`: the `draw_system.c` entry, if it describes the camera,
names the off one; one sentence under 300 characters.

New test `3d/tests/camera_switched_off.c`, needing no graphics card
(`voe_3d_draw_system_frame` takes no device). Its world is built as
`3d/tests/lights_switched_off.c`'s `world_of` builds one (read its lines
1-100 only), plus the parent table (`scene/parent_system.h`); its camera's
entity, with an identity, at (3, 2, 6), turned 30 degrees about +Y, and a
lens of fov 0.5, planes 0.5 and 200. Size 1280 by 720, lag 0. Cases:
1. On: `eye` is (3, 2, 6) and `view` is what `voe_3d_view`
   (`3d/projection.h`) gives for that pose and lens.
2. Off: `eye` is nought, `blind` false, and `view` is what `voe_3d_view`
   gives for the identity pose and the table's default row.
3. On again: as case 1.
4. The camera parented under an unmoved entity with an identity, the
   parent off: as case 2; the parent on: as case 1.
5. Off, framed with lag 0.5: still as case 2.
Its header lists the cases and names 0441. Entry in `3d/tests/tests.md`
beside `sky.c`'s, one sentence under 300 characters.

## Done when
The tests `3d/camera_switched_off`, `3d/sky`, `3d/draw_system` and
`3d/lights_switched_off` pass after the folder's build (those needing a
graphics card may skip).
