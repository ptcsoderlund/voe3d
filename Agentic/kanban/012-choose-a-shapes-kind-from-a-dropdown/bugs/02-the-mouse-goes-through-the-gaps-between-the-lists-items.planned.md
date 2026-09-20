# 02 — The mouse goes through the gaps between the list's items

## Seen
"There is space between all items, if i have my mouse over that space, the mouse goes through and triggers
mouse hover on the things under it. The popup should be filled and not allow cursor through."

The open list of kinds is drawn as separate item rows with a gap between them. Resting the cursor in one of
those gaps hovers whatever is underneath the list — the field or button the list is covering lights up as if
the list were not there.

## Expected
The open list is one filled, opaque area. Everywhere inside its outline — the gaps between items, the padding
at its edges — belongs to the list: nothing under it hovers, highlights or reacts to the cursor, and a click
anywhere inside it is the list's click, not the covered widget's. The cursor moving down the list from Cube to
Capsule to Cylinder never flickers anything underneath.

## How to reproduce
1. Open the editor on a project, select an entity with a Shape, and open the kind dropdown.
2. Move the cursor slowly down the list, from Cube to Cylinder, passing over the gaps between the items.
3. Whatever the list is covering hovers and highlights whenever the cursor is in a gap.
