# 18 — The open list takes the pointer over its whole outline
folder: editor
decisions: 0168, 0195, 0198, 0199
read: bugs/02-the-mouse-goes-through-the-gaps-between-the-lists-items.md

## Change
Two lines of the editor, and the bug is gone: card 17 made a container able to take the pointer, and the open
list is the container that wants it.

`editor/src/inspector.c`: in the static function card 12 added that draws the open list, the
`voe_ui_panel_begin(ui, "list", 0, VOE_UI_SURFACE_RAISED, ...)` container gains `.blocks_pointer = true`
beside its `across`, `gap` and `pad`. The panel and not the column that anchors it, because the panel is the
filled outline a person sees — its border, its surface and the padding inside them — and its visible rectangle
is what the Inspector's scroll area has left of it. Nothing else in the function changes.

`editor/src/inspector.h`: the paragraph THE OPEN LIST IS DRAWN ON THIS PANEL AND NOT OVER IT gains what is now
also true of it. Its panel takes the pointer (ui/layout.h), so everything inside its outline is the list's: the
gaps between the rows and the padding at its edges belong to it, and no field, button, swatch or number box it
covers hovers, highlights or fires through it (ADR-0199). And the consequence worth writing down rather than
rediscovering: a press that lands in that padding reaches no widget at all, so it is the press on nothing that
closes the list (inspector_edit.h) — it closes the list and disturbs nothing underneath, which is what a click
inside an overlay is for.

## Done when
The coder: `checks.sh --folder editor` exits 0, `cmake --build --preset debug` builds the whole tree, and
`checks.sh --all` exits 0. This is the last card of 012, and that one command is the whole-suite proof.

The human, at a running `voe_editor` on a project — the bug's own steps first: select an entity with a Shape,
open the kind dropdown where it covers other fields, and move the cursor slowly down the list from Cube to
Capsule to Cylinder, resting it in the gaps between the rows and in the padding at the list's edges. Nothing
under the list lights up, highlights or flickers anywhere on the way, and the row the cursor is on is the only
thing that changes. Then feature.md's steps 2, 3 and 8, which this card's two lines are the only thing between:
picking Cylinder changes the shape in both views at once and marks the project unsaved; Capsule and then Cube
each change it straight away and nothing else about it; Escape and a click somewhere else close it with the
kind unchanged. The rest of `## How to test` — adding a Shape, the colour and the Transform, Add → Capsule,
duplicating, saving and re-opening — is card 10's walkthrough and is untouched here; one pass through it
confirms it still reads the way it did.
