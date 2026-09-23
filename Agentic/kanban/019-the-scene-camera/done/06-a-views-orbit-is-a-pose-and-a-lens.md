# 06 — A scene view's orbit is a pose and a lens
folder: editor
decisions: 0168, 0222, 0223

## Change
Card 01 made the camera component a lens and card 02 made `3d` take a `voe_render_view`. A view's camera stays
the editor's own value, never an entity (0222).

- `editor/src/view.h` / `view.c` — `voe_editor_view`'s `voe_scene_camera camera` becomes `voe_math_float3 eye`,
  `float yaw`, `float pitch` (worked out by the orbit from focus and distance, as today) and
  `voe_scene_camera lens` (the lens's default numbers). Where view.c read the camera's forward, it works the
  forward out of yaw and pitch itself (zero yaw looks down −Z, positive yaw turns towards −X, positive pitch
  looks up). `voe_editor_view_pass_camera` builds its view with `voe_3d_view` (`3d/include/3d/projection.h`)
  from a pose — position the eye, rotation yaw about +Y then pitch about the turned X, scale one — and the lens;
  an orbit always sees, so a false is asserted. Header: the "camera is the editor's" paragraph says it is a
  pose and a lens turned into a render view, the way the world's camera is.
- `editor/src/pick.c` and `editor/src/gizmo.c` — `voe_3d_pick_ray` is handed
  `voe_editor_view_pass_camera(view, (voe_render_light){ 0 }).view` instead of the view's camera; `gizmo.c`
  already builds that value, so it passes the one it has.
- `editor/src/src.md` — the `view.h` entry if it names the camera.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`grep -nE "voe_scene_camera_view|camera_forward|view->camera|clicked->camera" editor/src/*.c` prints nothing.
