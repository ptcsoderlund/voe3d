# 01 — Scene list rows too far apart

## Seen
"The scene tree is having too much space between entities. You know, the list on the top left."
The rest of the editor looks right after 054; the Scene list still spaces its entity rows widely.

## Expected
The Scene list's rows sit as close as lines in a plain list: no visible gap between one entity's
row and the next beyond the text's own line height, so clearly more entities show in the same
height. Every row is still easy to click, drag and fold with the mouse, the drag marks still show
where a drop lands, and nested rows still read as a tree.

## How to reproduce
1. Start the editor on `examples/tank_game`.
2. Look at the Scene list, top left: the gap between one entity's row and the next is large
   compared with the Inspector's rows and the rest of the tightened editor.
