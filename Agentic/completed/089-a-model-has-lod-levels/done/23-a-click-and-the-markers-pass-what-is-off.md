# 23 — A click and the markers pass what is off
folder: 3d
after: 22
decisions: 0168, 0427, 0428

## Change
0428 points 7 and 8: a scene view's click and the editor's markers pass an
off entity by; cameras ignore the switch. `voe_scene_identity_off` is in
`scene/identity_component.h`. The editor's own pick only calls
`voe_3d_pick`, so the fix is here, once.

`3d/src/pick.c`: `keep_nearer` is where every walk (shapes, models, waters,
landscapes, cameras, suns, point lights, places) keeps a hit. It gains the
world as its first parameter and keeps nothing when the entity is off,
unless the entity has a camera row (`voe_scene_camera_get`), since cameras
ignore the switch. Every call in the file passes the world. Its comment
says both. `voe_3d_pick_landscape` (the brush's own question about the one
landscape it paints) is unchanged. The file's header gains "an off entity
is not met (0428)".

`3d/src/draw_marks.c`: the row loops of `voe_3d_draw_marks_point_lights`
(line 152), `voe_3d_draw_marks_places` (213), `voe_3d_draw_marks_sun` (309)
and `voe_3d_draw_marks_light_blockers` (409) skip an off owner. The camera's
marks, the outline, the collider's and the selected blocker's lines and the
gizmo are unchanged, so a selected off entity is still shown as selected.
The file's header gains the same phrase. `3d/src/draw_marks.h`'s comments
on those four functions say "an off owner's left out".

`3d/include/3d/pick.h`: `voe_3d_pick`'s comment gains that an off entity is
never the answer, a camera aside.

`3d/tests/pick.c`: its world (`a_world`) also registers the identity table
(`scene/identity_system.h`) and the parent table if it does not already,
and the cubes and markers the new cases use carry identities. New cases,
in `main`:
- `an_off_cube_lets_the_one_behind_be_met`: two cubes on the centre ray as
  `the_nearer_of_two_wins_in_either_order` sets them; the nearer off: the
  farther is met; its parent off instead (the nearer a child of a third
  entity): the farther is met.
- `an_off_sun_marker_is_not_met`: as `a_sun_marker_wins_over_the_cube`,
  the sun off: the cube is met.
- `a_camera_is_met_under_an_off_parent`: as
  `a_camera_marker_wins_over_the_cube`, the camera parented under an off
  entity: the camera is still met.
The file's header lists the three; `3d/tests/tests.md`'s `pick.c` entry
gains "and passes what is off", staying under 300 characters.

## Done when
The test `3d/pick` passes after the folder's build with the three new
cases, and `3d/far`, `3d/point_light_marker` and `3d/lights_switched_off`
still pass.
