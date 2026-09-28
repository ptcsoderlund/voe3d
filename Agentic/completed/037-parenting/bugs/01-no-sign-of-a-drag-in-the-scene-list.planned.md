# 01 — No sign of a drag in the Scene list

## Seen
Dragging a row in the Scene list shows nothing but the ordinary mouse-over
highlight. Nothing shows which row is being dragged or where it will land, and
there is no way to call the drag off once it has started.

## Expected
While a row is being dragged:
- A ghost label with the dragged thing's name follows the pointer.
- The dragged row is dimmed.
- The row under the pointer gets a drop highlight in the theme's accent
  colour, clearly different from the mouse-over highlight, when dropping there
  would make the dragged thing its child.
- The "Scene" heading gets the same drop highlight when the pointer is over
  it, because dropping there makes the dragged thing a root.
- A row that would refuse the drop (the dragged row itself, or anything under
  it) gets no drop highlight.
- Pressing Escape aborts the drag: the ghost and highlights go, nothing is
  reparented, nothing goes on the undo history, and releasing the button
  afterwards does nothing. The selection is what it was before the drag.
- Releasing the button ends the drag and everything goes back to normal. A
  plain click still just selects, with no ghost and no flash.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Press on the `tank_head` row in the Scene list and hold.
3. Move the pointer over `tank_body`, then over "Scene", then back over
   `tank_head` itself.
4. Only the ordinary mouse-over highlight shows. There is no ghost and no drop
   highlight.
5. Press Escape while still holding, then release over `tank_body`. The drag is
   not called off.
