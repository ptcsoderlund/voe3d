# 14 — The editor shows every directional light
folder: editor
after: 13
decisions: 0168, 0357, 0287, 0273, 0349

## Change
Every view and the preview light by every light, and Duplicate copies a light (decision 0357
points 1 and 5).

- `editor/src/view_passes.c`, in both the preview's draw and each view's:
  - after `voe_3d_draw_system_light_blockers`, call `voe_3d_draw_system_lights(world, &frame,
    arena)` (cast to void as its neighbours are);
  - the pass camera's `more` is the frame's further lights, through
    `voe_3d_draw_system_camera(&frame)` where the camera is built from the frame. A world with no
    light row has none, so the preview light stays alone.
  - Read the header of `3d/include/3d/draw_system.h` for the two calls. The header of
    `view_passes.c`: one point that every light draws, and that the preview light is never joined
    by another.
- `editor/src/view_passes.h`: `VOE_EDITOR_CAPACITIES`.
  - `passes`: per view and the preview, `VOE_RENDER_SHADOW_CASCADES ×
    VOE_RENDER_DIRECTIONAL_LIGHTS` cascades and `VOE_RENDER_DIRECTIONAL_LIGHTS` bounce shadow passes
    in place of 4 and 1.
  - `objects`: the casters' factor changes the same way.
  - The comment above it says so.
- `editor/src/view.h`: the light comments (lines ~35–40 and ~173–178) say the world's first light
  is what the views take as `light`, and that the others come with the frame.
- `editor/src/inspector.c` (line ~703) and `editor/src/scene.c` (`voe_editor_scene_duplicate`, line
  ~196): a light's entity gets Duplicate and is copied. The camera's is still refused. Update
  the comments there, `scene.c`'s header (line ~7), `editor/src/inspector.h` (line ~252) and
  `editor/src/scene.h` (line ~265).
- `editor/src/src.md`: only the entries whose
  claim changed.

## Done when
`[ $(grep -c voe_3d_draw_system_lights editor/src/view_passes.c) -eq 2 ] && grep -q
VOE_RENDER_DIRECTIONAL_LIGHTS editor/src/view_passes.h && ! grep -n "voe_scene_light_get"
editor/src/scene.c` exits 0. The human: the scene of `## How to test` step 7, every light deleted,
shows the preview light in the views.
