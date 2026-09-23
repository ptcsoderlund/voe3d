# 10 — Every scene the editor opens has its camera
folder: editor
decisions: 0168, 0218, 0222, 0223

## Change
Decision 0218: a scene has exactly one camera. Cards 01 and 06 made the component a lens and freed the
views of it; this gives every project's world its one camera.

- `editor/src/project.c` —
  - `world_new` registers the camera component (`scene/camera_component.h`) with room for one, after the
    transform it needs (card 01 asserts that order).
  - `build_untitled` adds a third entity beside the cube and the light: an identity named "Camera" with the
    next id the file gives its entities, a transform at (0, 2, 6) with yaw 0 and pitch −atan(2/6) (the pose
    card 06's view uses: yaw about +Y, then pitch about the turned X; scale one), and the default lens
    (60°, 0.1, 1000 — card 01's default row, not a second copy of the numbers: use the registered default).
  - `voe_editor_project_new_opened`, after the scene is read: when the world's camera count is 0, the same
    camera is added with an id one above the largest identity id in the world, and `project->unsaved` is set,
    so the top bar shows the scene is changed until it is saved. A scene that has one is left as read.
    One static helper adds the camera for both.
- `editor/src/project.h` — the `new_untitled` comment names three entities; `new_opened`'s says an old scene
  with no camera is given one and marked unsaved (0218). If the header's room paragraph counts entities, it
  counts the camera.
- `editor/src/src.md` — the `project.c` entry: the untitled scene's cube, light and camera.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/frame.png" --size 640x360` exits 0.
