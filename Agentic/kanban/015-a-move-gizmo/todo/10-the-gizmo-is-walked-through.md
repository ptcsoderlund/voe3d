# 10 — The gizmo is walked through
folder: editor
decisions: 0168, 0177, 0205, 0207, 0208
read: feature.md

## Change
No product code by default: this is the walk that proves the feature, and whatever the walk shows wrong.

Make the walk project outside the repository, in this run's scratchpad, in a folder of your own, say
`<scratch>/walk/`. Two files:

`project.voe3d`

    [project]
    scene = "main.scene"

`main.scene`, one section per entity and one per component, ids from 1, `kind` 1 cube, 2 capsule:

    [1]
    name = "Cube"
    [1.voe_scene_transform]
    position = [0, 0, 0]
    rotation = [0, 0, 0, 1]
    scale = [1, 1, 1]
    [1.voe_3d_shape]
    kind = 1
    colour = [0.6, 0.6, 0.6]

then the same five sections for `[2]` "Capsule" kind 2 at `[2.5, 0, 0]`, and

    [3]
    name = "Sun"
    [3.voe_scene_light]
    direction = [-0.4, -0.8, -0.45]
    colour = [1, 1, 1]
    intensity = 3

Check it loads and draws: `voe_editor <scratch>/walk --capture <scratch>/walk/frame.png --size 1280x720`
writes one frame with no window (ADR-0177). A capture selects nothing, so no gizmo is in it; it pins down a
drawing fault, it never passes the walk.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk`. A step that fails
is fixed here, in this folder and in at most one of `editor/src/gizmo.c`, `editor/src/view.c` and
`editor/src/main.c`; read the header of the one you touch and no other file.

A fault plainly another folder's is not fixed here: name it and block the card. Those are: a handle hit
where it is not drawn or drawn where it is not hit, or a size that changes with distance
(`3d/include/3d/gizmo.h`); a gizmo hidden by what it stands on, or a colour not the one handed in
(`3d/include/3d/draw_system.h`); a position a frame late or missing (`scene/transform_system.h`); a drag one
undo step too many or too few (`editor/src/undo.h`).

The findings older than 015 (header and entry caps across the repo) are not this card's: ADR-0208 proves
015 by its own folders and a cleanup feature clears the rest. Do not run or require `checks.sh --all`.

## Done when
The coder: the walk project loads and its capture shows both shapes;
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` and `... --folder editor` each exit 0; and
`cmake -P check.cmake` (which runs the whole ctest suite) exits 0.

The human, at a running `voe_editor`, sees all nine of `feature.md`'s steps hold: three arrows and three
squares on the selected cube in both views, over the cube and whatever is in front of it; each arrow moving
the cube along its axis alone and each square in its plane alone, live under the pointer with the Inspector
following; world X still world X after the cube is rotated; the gizmo about the same size on screen near
and far; an arrow marked one way hovered and another pressed, in the theme's lightness; a drag marking the
project unsaved and one Ctrl+Z putting the cube back where that drag began; a click off the handles
clearing selection and gizmo, and a click on the capsule bringing it there; a middle-drag begun on a handle
orbiting the camera while the cube stays put.
