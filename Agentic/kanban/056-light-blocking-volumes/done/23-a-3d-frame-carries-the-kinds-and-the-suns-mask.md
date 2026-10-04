# 23 — A 3d frame carries the kinds and the sun's mask
folder: 3d
after: 17, 22
decisions: 0168, 0250, 0348, 0350

## Change
`voe_3d_draw_system_light_blockers` fills 0350 point 2's words; the bounce takes them with
`frame->blockers` unchanged. Read the headers of `3d/src/draw_light_blockers.c`,
`3d/src/light_blocker.c`, `voe_3d_draw_system_light_blockers` in `3d/include/3d/draw_system.h`,
`scene/include/scene/light_blocker_component.h`, `scene/include/scene/light_component.h`, and
`voe_render_light_blockers` and `voe_render_light_blockers_mask` in
`render/include/render/device.h`.

- `3d/src/draw_light_blockers.c`: each kept record's bit (its index among the kept, not the
  table's) goes into `walls` or `indoors` by the row's kind, none for a Room; then `sun` is
  `voe_render_light_blockers_mask` over the kept records at the light row's entity's world place at
  `frame->lag`, about `frame->eye` in float, taken as light_blocker.c takes a blocker's place; no
  light table, no row or no transform is 0. Header points.
- `3d/include/3d/draw_system.h`: the call's comment names the kinds, the sun's mask and where it
  comes from.
- `3d/tests/light_blockers.c`: a Wall, an Indoors and a Room give their bits in `walls`,
  `indoors` and neither; a blocker left out shifts the bits after it; a light entity placed inside
  a blocker sets its bit in `sun`, outside sets none, no light row is 0. And a GPU case as the
  file's: a Wall box floating over a lit ground shape under a low sun with a fill, drawn through
  `_frame`, `_light_blockers` and `_run`: the ground in the box's shadow along the sun reads the
  fill, not black and below sunlit; ground clear of it reads sunlit.
- `3d/tests/bounce_scene.c`: only if a sample now lies where a probe's segment meets its blocker,
  that sample is moved so it does not, and why is said where it is.
- `3d/src/src.md`, `3d/tests/tests.md`, `3d/3d.md`: the entries for what changed. Each at most 300
  characters.

## Done when
The tests `3d/light_blockers`, `3d/bounce_scene` and `3d/draw_system` pass after the folder's
build.
