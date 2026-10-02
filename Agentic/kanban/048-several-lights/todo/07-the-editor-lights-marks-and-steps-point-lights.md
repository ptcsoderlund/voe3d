# 07 — The editor lights, marks and steps point lights
folder: editor
after: 06
decisions: 0168, 0274, 0320

## Change
Every editor view and the camera preview are lit by the point lights as the game is, every view marks
them, and their flashes run (0320 points 6, 7 and 9). Adding, editing, undo, Duplicate, parenting and
saving need no editor code: the component is described and has a menu path, and 0273's Duplicate
refusal names the sun only. Read the headers of `3d/include/3d/draw_system.h`
(`voe_3d_draw_system_point_lights`, `voe_3d_point_lights_marked`),
`3d/include/3d/point_light_marker.h`, `scene/include/scene/point_light_system.h`, and the files
below.

- `editor/src/world_step.c`: `voe_scene_point_light_system_run(world, seconds)` beside the emitter
  system's run; its header's list of what runs names it.
- `editor/src/view_passes.c`:
  - The preview: after its shadows, `voe_3d_draw_system_point_lights(world, &frame, arena)`; the
    positional pass camera at line 79 becomes designated and passes `.points = frame.points`; no
    marker (the preview marks nothing).
  - Each view: after its shadows the same call, `camera.points = frame.points` before the pass
    opens, and the frame handed to `voe_3d_draw_system_run` carries `.point_lights` shown, with the
    selection, the outline material, `marker_colour`'s rest colour and the outline colour for the
    selected, and the sun marker's pixels and size.
  - A false from the call leaves that pass unlit by points and does not fail the frame.
  - The file's header and `marker_colour`'s comment name the point light markers.
- `editor/src/view_passes.h`, `VOE_EDITOR_CAPACITIES`: the per-view transient vertices and indices
  gain `VOE_3D_POINT_LIGHT_MARKER_VERTICES` / `_INDICES` × `VOE_GAME_WORLD_POINT_LIGHTS`; the
  per-view objects term rises by 2 and the transient geometries per view from 6 to 8; the reasoning
  comment above it says why. Include `3d/point_light_marker.h` there.
- `editor/src/src.md`: the world_step.c, view_passes.c and view_passes.h entries name point lights.
  Each at most 300 characters.

## Done when
After the folder's build, `grep -c voe_3d_draw_system_point_lights editor/src/view_passes.c` prints
2, `grep -q voe_scene_point_light_system_run editor/src/world_step.c` exits 0, and
`grep -q VOE_3D_POINT_LIGHT_MARKER_VERTICES editor/src/view_passes.h` exits 0.

The human's: open `examples/tank_game` in the editor, Add component "Rendering / Point light" on a
new entity near the ground; its marker shows in every view and the ground under it is lit; change its
colour, strength and reach in the Inspector and the pool changes at once; undo works.
