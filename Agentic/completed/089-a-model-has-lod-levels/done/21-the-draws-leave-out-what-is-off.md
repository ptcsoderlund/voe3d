# 21 — The draws leave out what is off
folder: 3d
after: 15, 16, 17, 19
decisions: 0168, 0427, 0428

## Change
0428 point 3: whatever is off is left out of every draw where the frame's
`hidden` is left out today.

`3d/src/draw_group.h` / `3d/src/draw_group.c`: new
`bool voe_3d_draw_group_left_out(const voe_ecs_world *world, voe_ecs_entity owner, voe_ecs_entity hidden);`
true when `owner` is `hidden` (`voe_3d_draw_group_is_the_same_entity`) or
`voe_scene_identity_off(world, owner)` (`scene/identity_component.h`).
Comment above it: the one test a draw makes of a row's owner; why the
switch is asked here and not kept (0428 point 2). `is_the_same_entity`
stays: `3d/src/bounce_casters.c` uses it to compare casters, not to hide.

Replace each `voe_3d_draw_group_is_the_same_entity(<owner>, <frame>.hidden)`
with `voe_3d_draw_group_left_out(world, <owner>, <frame>.hidden)`, the rest
of each condition unchanged:
- `3d/src/draw_system.c` lines 411, 468, 629, 696 (models, landscapes,
  meshes, panels). The file is 780 lines: open only those lines and
  their function's head for the world's name, nothing else.
- `3d/src/draw_shadows.c` 122, 190.
- `3d/src/draw_bounce.c` 313, 339, 384, 416. An off caster thus leaves the
  casters and `3d/src/bounce_casters.c` clears where it was, as it does a
  hidden one; no change there.
- `3d/src/draw_particles.c` 137; `3d/src/draw_water.c` 107.
If a site's function has no `world` in scope, it is passed in from its one
caller in the same file; say so in that function's comment.

Comments: `3d/src/draw_particles.h` line 32, `3d/src/draw_water.h` line 40,
`3d/src/draw_bounce.h` line 73 and `3d/src/draw_shadows.c` line 18 say
"and what is switched off" beside the hidden entity. `3d/include/3d/draw_system.h`
is at 59 of its 60 header lines: do not grow its header. The `hidden`
field's comment (about line 310) and the shadows and run comments that
name it (about lines 617 and 719) gain the phrase that switched-off owners
are left out the same way (0428).

`3d/src/src.md`: the `draw_group.c` entry names the left-out test, one
sentence under 300 characters.

New test `3d/tests/draw_switched_off.c`, built as `3d/tests/draw_levels.c`
is (camera, casting sun, the sphere from `3d/tests/model_glb.inc`, a whole
frame's `voe_render_frame_triangle_count`, skipping with a reason without a
graphics card), its world also registering the identity table
(`scene/identity_system.h`) and the parent table (`scene/parent_system.h`):
1. A frame with no model row: B.
2. A parent entity with an identity, the sphere's model row on its child
   (with an identity too), 3 m before the camera: N, over B.
3. The child's identity submitted `off = true` and the system run: B.
4. The child back on and the parent off: B.
5. The parent back on: N.
Its header says the cases and why the counts compare to B. Entry in
`3d/tests/tests.md`, one sentence under 300 characters.

## Done when
The test `3d/draw_switched_off` passes after the folder's build (or skips
for want of a graphics card), and `3d/draw_levels`, `3d/draw_system` and
`3d/shadows` still pass.
