# 08 — A 3d bounce keeps to its blockers
folder: 3d
after: 05, 07
decisions: 0168, 0347

## Change
The frame's blockers reach the probe bounce (0347 point 4). Read the headers of
`3d/src/draw_bounce.h`, `3d/src/draw_bounce.c`, `voe_3d_draw_system_shadows` and
`voe_3d_draw_system_light_blockers` in `3d/include/3d/draw_system.h`, `struct
voe_render_bounce_frame` in `render/include/render/device.h`, and `3d/tests/bounce_scene.c`.

- `3d/src/draw_bounce.c`: the bounce frame it begins carries `.blockers = frame->blockers`.
  Header point: the bounce keeps to the frame's blockers, filled before this call.
- `3d/include/3d/draw_system.h`: `voe_3d_draw_system_shadows`' comment says the bounce is handed
  `frame->blockers`, so `voe_3d_draw_system_light_blockers` comes first, and a blocker that changes
  relights.
- `3d/tests/bounce_scene.c`: a case through the editor's and game's calls: the red box tinting the
  ground it faces, then a light blocker (table registered, row added) around a patch of that ground
  and not the box: once settled the patch reads within 2/255 of bounce off, ground outside it reads
  as before; the blocker removed, the tint comes back once settled.
- `3d/src/src.md`, `3d/tests/tests.md`: the draw_bounce.c and bounce_scene.c entries name
  blockers. Each at most 300 characters.

## Done when
The tests `3d/bounce_scene` and `3d/light_blockers` pass after the folder's build.
