# 02 — Dragging a prefab shows no ghost, and no drag says when it cannot drop

## Seen
"I always want ghost label on drag and drop, prefabs dont have that. If i cannot drop something on
a panel, ghost label should indicate that."

Dragging a prefab from the Assets panel shows no ghost with its name, unlike a Scene list drag
(0282). No drag shows anything when the pointer is over a panel that will not take it.

## Expected
Decision 0285: every drag, including a prefab or model dragged from the Assets panel and a thing
dragged from the Scene list, shows the ghost with the dragged thing's name. Over a place that will
not take it, the ghost is dimmed and says "Can't drop here", and letting go there drops nothing.
Over a place that takes it, the ghost looks normal.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Drag a prefab from the Assets panel toward a view. No ghost follows the pointer.
3. Drag it over the Scene list or the Inspector (places a prefab cannot be dropped). Nothing says
   it cannot go there.
4. Drag a thing from the Scene list over the Inspector. A ghost shows, but nothing says it cannot
   be dropped there.
