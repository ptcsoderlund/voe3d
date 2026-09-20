# 23 — The fit is written down and walked through
folder: editor
decisions: 0168, 0177, 0195, 0198, 0199, 0200
read: feature.md

## Change
The last card of 012: the folder's two maps say what the list now does, and the whole feature is walked.

`editor/src/src.md`, the entries only, each one sentence in the voice it already has:

- `dock.c` — the walk hands the Inspector's leaf the scroll area it opens, for the open list to fit itself into.
- `inspector.h` and `inspector.c` — the open list's rows sit in a scroll area of their own, at their natural
  height or capped to the room the panel leaves, and the panel is told which area clips it.
- `inspector_edit.h` and `inspector_edit.c` — the list's side and its cap are worked out from the button's
  rectangle and that area's, every frame it is open, and only a press outside its outline closes it.
- `scene.h` and `scene.c` — the place the list is moved to carries how tall its rows may be.
- `interface.c` — it hands the buttons' read the pointer's place as well as its button.

`editor/editor.md`: the Inspector's sentence about a named field's dropdown says that the list opens where it
fits — under the button, above it when there is no room under, and capped to the room and scrolling inside
itself when there is room for it on neither side — so the last value can always be reached and picked (0200).

Then the walk, which writes nothing by default. Run the eight steps of `feature.md`'s `## How to test` in a
built `voe_editor` on a project saved somewhere of your own, and the reproduction steps of all three files in
`bugs/`. A step that does not hold is fixed here, in this folder and in at most one of
`editor/src/inspector.c`, `editor/src/inspector_edit.c`, `editor/src/interface.c`, `editor/src/dock.c` and
`editor/src/scene.c` — read the header of the one you touch first and no other file. A fault that is plainly
`ui`'s (a rectangle or a clip that is not what ui/layout.h says it is) or `3d`'s (a kind that lands in the row
but draws the old shape) is not fixed here: say so and the card is blocked.

One thing that is not a fault and is not fixed: what a capped list cannot take of a wheel gesture passes
outward to the Inspector's own area behind it, which is `ui`'s rule for nested areas (ui/widgets.h, THE SCROLL
AREA). The panel then scrolls, its button moves, and the list follows the button and re-picks its side — nothing
is cut off and nothing is lost.

## Done when
The coder: `checks.sh --all` exits 0.

The human, at a running `voe_editor`, sees every one of `feature.md`'s eight steps hold — Add → Entity,
Transform and Shape giving a grey cube at the origin whose Shape section reads Cube; the list opening on Cube,
Capsule and Cylinder and Cylinder turning the cube into a cylinder in both views at once with the project marked
unsaved; Capsule and Cube again each changing the drawn shape at once and the place, size and colour never; a
shape given a colour and moved keeping both across a change of kind; an entity added with Add → Capsule reading
Capsule and changing the same way; a duplicate of a changed entity being the same kind; every kind as it was
left after a Save, a close and an open of the same folder; and Escape or a click elsewhere closing the list with
the kind unchanged.

And all three bugs gone: scrolling the panel with the list open keeps the list glued under — or over — its
button and clipped with it (bug 01); the cursor moving down the rows and resting in the gaps lights nothing up
underneath (bug 02); and a dropdown near the very bottom of a scrolled Inspector opens above its button, whole,
with the last kind on it and pickable, or capped and scrolling to it when there is room on neither side (bug
03).
