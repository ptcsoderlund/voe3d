# 09 — Add entity, then Add component, is walked through
folder: editor
decisions: 0168, 0177, 0204, 0217, 0221
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever it shows to be wrong.

Make the walk project outside the repository, in this run's scratchpad, say `<scratch>/walk/`:
`project.voe3d` holding `[project]` and `scene = "main.scene"`, and `main.scene` holding one sun:

    [1]
    name = "Sun"
    [1.voe_scene_light]
    direction = [-0.4, -0.8, -0.45]
    colour = [1, 1, 1]
    intensity = 3

Check it loads: `voe_editor <scratch>/walk --capture <scratch>/walk/frame.png --size 1280x720` writes one
frame with no window (ADR-0177). A capture presses nothing; it pins a drawing fault, never passes the walk.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk` and reports what does
not hold. A failing step is fixed here, in at most one of `editor/src/add_menu.c`, `inspector.c`,
`inspector_edit.c`, `entities.c` and `scene.c`; read the header of the one you touch and no other file. A fault
plainly another folder's is not fixed here: the card is blocked, naming it — a path not registered
(`ecs/include/ecs/component.h`, `scene/`, `3d/`), a row a system does not pick up (`3d/shape_system.h`,
`ecs/structure.h`), a scene text that does not come back (`authoring/scene_write.h`,
`authoring/scene_read.h`).

## Done when
The coder: the capture above is written, and `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every step of `feature.md` hold: no button makes a shape; Add entity
lists and selects "Entity" with only a transform at 0, 0, 0 and nothing drawn; Add component opens a list
with Rendering and Transform absent (the entity has one), Rendering opening Light and Shape beside it; Shape
draws a cube at the centre with its kind and colour in the Inspector; a capsule and a new colour are followed
by the view; Shape is not offered again; the transform has no Remove; removing the shape leaves the entity
listed; three undos take back the removal, the add and the entity in that order and redo restores them; a
saved, closed and reopened project is as it was left.
