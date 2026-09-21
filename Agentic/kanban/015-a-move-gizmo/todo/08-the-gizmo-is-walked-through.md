# 08 — The gizmo is walked through
folder: editor
decisions: 0168, 0177, 0205, 0207
read: feature.md

## Change
This card writes no product code by default: it is the walk that proves the feature, and whatever that walk
shows to be wrong.

First make the project the walk is run on, outside the repository, in this run's scratchpad folder — two
shapes and a light, so there is something to drag and something else to click on. Write two files in a folder
of your own, say `<scratch>/walk/`:

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

then the same five sections again for `[2]` "Capsule" kind 2 at `[2.5, 0, 0]`, and

    [3]
    name = "Sun"
    [3.voe_scene_light]
    direction = [-0.4, -0.8, -0.45]
    colour = [1, 1, 1]
    intensity = 3

Check it loads and draws before handing it over: `voe_editor <scratch>/walk --capture <scratch>/walk/frame.png
--size 1280x720` writes one frame with no window (ADR-0177). A capture presses nothing and selects nothing, so
no gizmo is in it; it is how a drawing fault is pinned down, never how the walk is passed.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk` and reports what does
not hold. A step that fails is fixed here, in this folder and in at most one of `editor/src/gizmo.c`,
`editor/src/view.c` and `editor/src/main.c` — read the header of the one you touch and no other file.

A fault that is plainly another folder's is not fixed here: say so and the card is blocked. Those are the ones
to name — a handle hit where it is not drawn or drawn where it is not hit, and a size that changes with
distance (`3d/include/3d/gizmo.h`); a gizmo hidden by the thing it stands on, or a colour that is not the one
handed in (`3d/include/3d/draw_system.h`); a position that arrives a frame late or not at all
(`scene/transform_system.h`); a drag that is one undo step too many or too few (`editor/src/undo.h`).

## Done when
The coder: the walk project above loads and its capture shows both shapes, and `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s nine steps hold — three arrows and
three squares on the selected cube in both views, over the cube and over whatever is in front of it; each
arrow moving the cube along its own axis alone and each square in its own plane alone, live under the pointer
with the Inspector following; the world's X still the world's X after the cube is rotated; the gizmo about the
same size on screen orbited close and far; an arrow marked one way hovered and another pressed, in the theme's
lightness; a drag marking the project unsaved and one Ctrl+Z putting the cube back where that drag began; a
click off the handles clearing the selection and the gizmo with it, and a click on the capsule bringing it
there; and a middle-drag begun on a handle orbiting the camera while the cube stays where it is.
