# 06 — Undo and redo are walked through
folder: editor
decisions: 0168, 0177, 0204
read: feature.md

## Change
This card writes no product code by default: it is the walk that proves the feature, and whatever that walk
shows to be wrong.

First make the project the walk is run on, outside the repository, in this run's scratchpad folder — two
shapes and a light, so there is something to drag, rename, recolour and delete. Write two files in a folder of
your own, say `<scratch>/walk/`:

`project.voe3d`

    [project]
    scene = "main.scene"

`main.scene`, one section per entity and one per component, ids from 1, `kind` 1 cube, 2 capsule, 3 cylinder:

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
--size 1280x720` writes one frame with no window (ADR-0177) and both shapes are in it. A capture presses no
keys; it is how a drawing fault is pinned down, never how the walk is passed.

Then the human walks `feature.md`'s nine steps at a running `voe_editor <scratch>/walk`, with a second copy of
the folder for step 9's other project, and reports what does not hold. A step that fails is fixed here, in
this folder and in at most one of `editor/src/undo.c`, `editor/src/project.c` and `editor/src/main.c` — read
the header of the one you touch (`undo.h`, `project.h`, or main.c's own) and no other file.

A fault that is plainly another folder's is not fixed here: say so and the card is blocked. Those are the
ones to name — a key that never reads as pressed (`platform/include/platform/input.h`), a scene text that
comes back as something other than what was written (`authoring/scene_write.h`, `authoring/scene_read.h`), a
row that a system does not pick up again after it is put back (`3d/shape_system.h`, `ecs/structure.h`).

## Done when
The coder: the walk project above loads and its capture shows both shapes, and `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s nine steps hold — a dragged X undone in
one step and redone by Ctrl+Shift+Z and by Ctrl+Y alike; a typed name, a picked colour and a changed kind
undone one at a time, newest first, with both scene views and the Inspector showing each at once; a deleted
entity back in `Scene` with its name, its id and every component; an add, a duplicate, a component added and
one removed undone one at a time and redone the same way; a new change after two undos leaving Ctrl+Shift+Z
with nothing to do; selecting different entities between changes never being what a Ctrl+Z undoes; Ctrl+Z
inside a number box being typed into leaving the history untouched; a Save followed by Ctrl+Z undoing the
change before it and leaving the project marked unsaved; and Ctrl+Z doing nothing at all in a project just
opened.

## Blocked
The walk project is made and proven — `<scratch>/walk` loads and its capture shows the cube and the capsule,
with a second copy at `<scratch>/walk2` for step 9 — but the nine steps need a person at the keyboard of a
running `voe_editor`, and a coder never presses keys in a window, so not one of them is seen and nothing has
been shown to be wrong to fix. `checks.sh --all` cannot exit 0 in this environment either: 40 of its 41
findings are header comments over the default 60-line cap in folders this card may not touch, and the last is
`cmake -P check.cmake`, which fails at every standalone configure because the repository sits on a root-owned
WSL drvfs mount where CMake's `configure_file` gets "Operation not permitted" — configuring the same folder
into an ext4 directory succeeds. `checks.sh --folder editor` has one finding, `editor.md`'s opening at 5443
characters against the 400 cap, which predates this feature (5009 at the branch point) and which card 05 was
told to add to. The human walking the nine steps, and a tree whose caps and mount the product's own checks can
meet, unblock it.
