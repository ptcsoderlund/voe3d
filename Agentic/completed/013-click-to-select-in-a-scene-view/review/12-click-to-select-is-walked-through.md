# 12 — Click to select is walked through
folder: editor
decisions: 0168, 0177, 0194, 0202, 0203
read: feature.md

## Change
This card writes no product code by default: it is the walk that proves the feature, and whatever that walk
shows to be wrong.

First make the project the walk is run on, outside the repository, in this run's scratchpad folder — five
shapes of three kinds with two of them standing behind others from a view's angle, and a light so they are not
flat. Write two files in a folder of your own, say `<scratch>/walk/`:

`project.voe3d`

    [project]
    scene = "main.scene"

`main.scene`, one section per entity and one per component, ids from 1, `kind` 1 cube, 2 capsule, 3 cylinder:

    [1]
    name = "Front cube"
    [1.voe_scene_transform]
    position = [0, 0, 0]
    rotation = [0, 0, 0, 1]
    scale = [1, 1, 1]
    [1.voe_3d_shape]
    kind = 1
    colour = [0.6, 0.6, 0.6]

and the same shape of five sections again for `[2]` "Behind capsule" kind 2 at `[0, 0, -3]`, `[3]` "Cylinder
left" kind 3 at `[-2.5, 0, 0]`, `[4]` "Cube right" kind 1 at `[2.5, 0, 0.5]`, `[5]` "Capsule high" kind 2 at
`[0.4, 1.6, 1.2]`, each with the same rotation, scale and colour; then

    [6]
    name = "Sun"
    [6.voe_scene_transform]
    position = [0, 0, 0]
    rotation = [0, 0, 0, 1]
    scale = [1, 1, 1]
    [6.voe_scene_light]
    direction = [-0.4, -0.8, -0.45]
    colour = [1, 1, 1]
    intensity = 3

Check it loads and draws before handing it over: `voe_editor <scratch>/walk --capture <scratch>/walk/frame.png
--size 1280x720` writes one frame with no window (ADR-0177) and the shapes are in it. A capture has nothing
selected and so carries no outline; it is how a drawing fault is pinned down, never how the walk is passed.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk`, and reports what does
not hold. A step that fails is fixed here, in this folder and in at most one of `editor/src/pick.c`,
`editor/src/view.c`, `editor/src/scene.c` and `editor/src/main.c` — read the header of the one you touch
(`pick.h`, `view.h`, `scene.h`, or main.c's own) and no other file.

A fault that is plainly `3d`'s is not fixed here: say so and the card is blocked. Those are the ones to name —
a click that answers an entity the pointer is not over, or none when it is over one (3d/pick.h); an outline
that is in the wrong place, the wrong thickness, missing edges, or hidden by what is in front of it
(3d/outline.h, 3d/draw_system.h). A fault in which view, which pixel or which colour is this folder's.

## Done when
The coder: the walk project above loads and its capture shows the five shapes, and `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s nine steps hold — a left click on a
shape in the top view marking its row in `Scene`, filling the Inspector and outlining it in both views; a click
on the visible part of a shape standing behind another selecting the one the pointer is actually over; a click
on empty space clearing the selection, the outline and the Inspector; a row picked in `Scene` outlining the
same entity in both views; a selected shape's outline still showing when the camera is moved so another shape
stands in front of it; a middle-button drag that starts over a shape orbiting the camera and changing nothing
about what is selected; the outline following the selected shape as its kind, colour and position are edited;
the outline staying clearly visible, grey or the theme's own hue and never a colour of its own, in Near white
and in one more theme; and a click in a view behind the open file browser selecting nothing.
