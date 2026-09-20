# 10 — Click to select is walked through
folder: editor
decisions: 0168, 0177, 0194, 0202, 0203
read: feature.md

## Change
This card writes nothing by default: it is the walk that proves the feature, and whatever that walk shows to be
wrong.

Run the nine steps of `feature.md`'s `## How to test` in a built `voe_editor`, on a project of your own holding
four or five shapes of different kinds, some of them behind others from a view's angle, saved in a folder so
step 9 has something to open the browser onto. A step that does not hold is fixed here, in this folder and in
at most one of `editor/src/pick.c`, `editor/src/view.c`, `editor/src/scene.c` and `editor/src/main.c` — read
the header of the one you touch first (`pick.h`, `view.h`, `scene.h`, and main.c's own) and no other file.

A fault that is plainly `3d`'s is not fixed here: say so and the card is blocked. Those are the ones to name —
a click that answers an entity the pointer is not over, or none when it is over one (3d/pick.h); an outline
that is in the wrong place, the wrong thickness, missing edges, or hidden by what is in front of it
(3d/outline.h, 3d/draw_system.h). A fault in which view, which pixel or which colour is this folder's.

`voe_editor --capture <path.png> --size 1280x720` draws one frame with no window and writes it (ADR-0177);
that is how a drawing fault is pinned down, not how the walk is passed — a capture has nothing selected and so
no outline in it.

## Done when
The coder: `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s nine steps hold — a left click on a
shape in the top view marking its row in `Scene`, filling the Inspector and outlining it in both views; a click
on the visible part of a shape standing behind another selecting the one the pointer is actually over; a click
on empty space clearing the selection, the outline and the Inspector; a row picked in `Scene` outlining the
same entity in both views; a selected shape's outline still showing when the camera is moved so another shape
stands in front of it; a middle-button drag that starts over a shape orbiting the camera and changing nothing
about what is selected; the outline following the selected shape as its kind, colour and position are edited;
the outline staying clearly visible, grey or the theme's own hue and never a colour of its own, in Near white
and in one more theme; and a click in a view behind the open file browser selecting nothing.
