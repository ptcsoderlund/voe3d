# 10 — Choosing a kind from the dropdown is walked through
folder: editor
decisions: 0168, 0177, 0195, 0198
read: feature.md

## Change
This card writes nothing by default: it is the walk that proves the feature, and whatever that walk shows to be
wrong.

Run the eight steps of `feature.md`'s `## How to test` in a built `voe_editor`, on a project saved somewhere of
your own so that step 7 has a folder to reopen. A step that does not hold is fixed here, in this folder and in at
most one of `editor/src/inspector.c`, `editor/src/inspector_edit.c`, `editor/src/interface.c` and
`editor/src/scene.c` — read the header of the one you touch first (`inspector.h`, `inspector_edit.h`,
`interface.h`, `scene.h`) and no other file. A fault that is plainly `3d`'s (a kind that lands in the row but
draws the old shape) or `base`'s (a description that carries no names) is not fixed here: say so and the card is
blocked.

`voe_editor --capture <path.png> --size 1280x720` draws one frame with no window and writes it (ADR-0177); that is
how a drawing fault is pinned down, not how the walk is passed — a captured frame has nothing selected and so no
Inspector section to look at.

## Done when
The coder: `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s eight steps hold — Add → Entity, Transform
and Shape giving a grey cube at the origin whose Shape section reads Cube; the list opening on Cube, Capsule and
Cylinder and Cylinder turning the cube into a cylinder in both views at once with the project marked unsaved;
Capsule and Cube again each changing the drawn shape at once and the place, size and colour never; a shape given
a colour and moved keeping both across a change of kind; an entity added with Add → Capsule reading Capsule and
changing the same way; a duplicate of a changed entity being the same kind; every kind as it was left after a
Save, a close and an open of the same folder; and Escape or a click elsewhere closing the list with the kind
unchanged.
