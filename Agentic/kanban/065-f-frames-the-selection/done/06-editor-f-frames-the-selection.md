# 06 — F frames the selection in the view under the pointer
folder: editor/src
after: 02, 05
decisions: 0168, 0371
read: feature.md

## Change
Act on F: the view under the pointer glides to the selection (decision 0371 points 2 and 3).

- `editor/src/frame_selection.h` (new) and `editor/src/frame_selection.c` (new) —
  `void voe_editor_frame_selection(voe_editor_views *views, const voe_editor_scene *scene,
  const voe_3d_shape_geometries *geometries, const voe_3d_models *models,
  voe_math_float2 pointer)`: does nothing when `scene->selected` is not alive
  (`voe_ecs_entity_alive`) or the pointer is over no view (`voe_editor_views_under`). Otherwise
  `voe_3d_bounds` (3d/bounds.h) on the selection; with a size, the focus is its centre and the
  distance `voe_3d_bounds_distance(radius, lens fov_y, view width / height, 2/3)`; without one,
  an entity with a transform gets its `voe_scene_transform_world` position and
  `VOE_EDITOR_FRAME_NEAR` (3 m), and one with no transform nothing. Then
  `voe_editor_view_glide_to` on that view only. Header points: why this is the editor's and the
  size `3d`'s; why only the view under the pointer; why a sizeless thing stops at a fixed distance
  with its marker in the middle; that nothing is saved or put on undo.
- `editor/src/frame_pointer.c` — straight after `voe_editor_frame_commands_read`, when
  `shortcuts.frame_selection` is set (the commands struct's `shortcuts`), call it with the views,
  the scene, the geometries, the store and the pointer in millimetres — the same ones the pick
  gets. `frame_pointer.h`'s header names F among what the run carries out.
- `editor/src/src.md` — entries for `frame_selection.h` and `frame_selection.c`.

Headers to read for the calls: `editor/src/view.h`, `editor/src/scene.h`,
`3d/include/3d/bounds.h`, `scene/include/scene/transform_component.h`.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0.
- `./build/debug/editor/voe_editor --capture build/debug/065-frame.png` exits 0.
- Human, every step of feature.md's How to test: open the tank game's scene; F on the tank in the
  top view glides there, angle kept, bottom view still; orbit turns about it; F in the bottom view
  on a small far thing; the ground nearly all in view; a light and the camera stop close, marker
  centred; a parent with a model child frames both; no selection, F moves nothing; F typed in an
  Inspector field goes in the field; F while flying frames nothing; reopened, the views start on
  the scene's camera.
