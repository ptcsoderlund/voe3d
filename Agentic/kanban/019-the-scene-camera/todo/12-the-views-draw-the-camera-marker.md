# 12 — The scene views draw the camera marker
folder: editor
decisions: 0168, 0203, 0205, 0223

## Change
Card 08 gave `voe_3d_frame` a `marker` (`voe_3d_camera_marked`, `3d/include/3d/draw_system.h`); card 09 put
the per-view passes in `editor/src/view_passes.c`. This sets the marker on every scene view's pass.

- `editor/src/view_passes.c` — in each view's pass, beside where the outline and the gizmo are set:
  `frame.marker` is the world's camera entity (the first of `voe_scene_camera_entities`, none when the count
  is 0), the same unlit material the outline uses, the pixel width the outline is drawn at, the view's target
  size, and as colour the outline's colour when that entity is the selected one and the gizmo's rest colour
  otherwise — the values the pass already hands the outline and the gizmo, not new constants (0223).
- `editor/src/view_passes.h` — `VOE_EDITOR_CAPACITIES` grows by one marker per view's pass:
  `VOE_3D_CAMERA_MARKER_VERTICES` and `_INDICES` (`3d/include/3d/camera_marker.h`) in the transient
  vertices and indices, one more transient geometry and one more object, each times `VOE_EDITOR_VIEWS`, and
  the capacity comment says so. The header's summary of what a pass draws names the marker.
- `editor/src/src.md` — the `view_passes.c` entry names the marker.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/frame.png" --size 640x360` exits 0 with nothing on
stderr about a capacity.
