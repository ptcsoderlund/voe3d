# 021 — Resize the editor's panels

## What
The borders between the view and the Scene list on the left, the Inspector on the right and the top bar can be
dragged to make those panels wider, narrower, taller or shorter. The pointer shows it is over a border you can
drag. A panel cannot be dragged smaller than a minimum that keeps it usable, and the view always keeps some
room. Double-clicking a border puts that panel back at its default size. The sizes are remembered for you in
the editor's own settings, not in the project, so every project opens with the panels as you last left them.

## Why
Decision 0220. The fixed sizes do not suit every screen or every task.

## How to test
1. Open the editor. Hover the border between the Scene list and the view; the pointer changes.
2. Drag it right. The Scene list widens and the view narrows as you drag. Drag it far left; it stops at a
   minimum width and the list is still usable.
3. Do the same with the Inspector's border and the top bar's border.
4. Try to make the side panels so wide that they meet. The view keeps some room.
5. Double-click one border. That panel returns to its default size; the others stay.
6. Close and reopen the editor. The panels are as you left them.
7. Open another project. The panels are the same sizes there.
