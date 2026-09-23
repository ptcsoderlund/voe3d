# 19 — The scene camera is walked through
folder: editor
decisions: 0168, 0177, 0218, 0222, 0223
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever it shows to be wrong.

Make the walk project outside the repository, in this run's scratchpad, say `<scratch>/walk/`:
`project.voe3d` holding `[project]` and `scene = "main.scene"`, and `main.scene` holding one sun and no
camera, as a scene saved before this feature:

    [1]
    name = "Sun"
    [1.voe_scene_light]
    direction = [-0.4, -0.8, -0.45]
    colour = [1, 1, 1]
    intensity = 3

Check it loads: `voe_editor <scratch>/walk --capture <scratch>/walk/frame.png --size 1280x720` writes one
frame with no window (ADR-0177) and prints no capacity line on stderr. (A run before cards 16–18 already
wrote it cleanly and trimmed the `view.h` entry in `editor/src/src.md`; the `3d` index findings that
stopped `checks.sh --all` are fixed by those cards.) A capture presses nothing; it pins a
drawing fault, never passes the walk.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk` (step 1 by New) and
reports what does not hold. A failing step is fixed here, in at most one of `editor/src/project.c`,
`scene.c`, `inspector.c`, `dock.c`, `view.c` and `view_passes.c`; read the header of the one you touch and no
other file. A fault plainly another folder's is not fixed here: the card is blocked, naming it — a lens the
drain refuses or does not replace (`scene/camera_component.h`), a view built wrong from a pose or a marker
drawn or hit wrong (`3d/projection.h`, `3d/camera_marker.h`, `3d/pick.h`, `3d/draw_system.h`), a camera
section that does not come back from a save (`authoring/scene_write.h`, `authoring/scene_read.h`).

## Done when
The coder: the capture above is written, and `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every step of `feature.md` hold: New lists "Camera" and both views
draw its box and frustum lines; the old scene opens with a camera and the top bar says unsaved; a click on the
camera's box selects it and the corner picture shows what it sees, in both views; a gizmo drag moves the box
and the picture follows; a typed rotation turns the lines and the picture, a typed field of view widens or
narrows both; Delete, Ctrl+D and the Inspector offer no way to delete, duplicate or remove its camera; Add
component on another entity offers no camera; undo and redo take back and restore the move and the rotation;
a saved, closed and reopened project has the camera where it was left, aimed the same way.
