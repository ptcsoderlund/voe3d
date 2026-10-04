# 061 — The Scene list shows what I selected

## What
When I select something anywhere but in the Scene list itself (clicking it in a scene view, Add entity,
Duplicate, dropping a model from Assets, undo or redo), the Scene list unfolds every folded parent above it and
scrolls so its row is in sight, centred. If the row is already in sight the list does not move. The parents stay
unfolded afterwards and a save keeps them so; the unfolding is not something undo steps back through (0355).

## Why
In a big scene a thing selected in the view is hard to find in the list, buried under folded parents.

## How to test
1. Open the tank game. Fold every parent in the Scene list and scroll it to the top.
2. Click a part deep inside a prefab in the scene view (for example a gun on the tank). Every parent above it
   unfolds, the list scrolls, and its row is selected in the middle of the list.
3. Click another thing whose row is already in sight. The list does not scroll.
4. Press Ctrl+Z. Undo steps back your last edit, not the unfolding; the parents stay open.
5. Duplicate the selected thing, and drop a model from Assets into the view. Each time the new row is in sight
   and selected.
6. Save, close and reopen the scene. The parents that were unfolded are still unfolded.
