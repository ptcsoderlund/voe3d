# 01 — The Assets panel cannot be resized vertically

## Seen
"Asset panel is not resizable vertically. I want it to be." The border between the Scene list and
the Assets panel below it does not move when dragged.

## Expected
That border behaves like every other panel border in the editor (features 021 and 022):
- Dragging it up or down moves it, so the Scene list and the Assets panel trade height.
- It rests in the border colour and lights up when the pointer reaches it or while it is held.
- Neither panel can be dragged smaller than a usable height.
- A double-click on it puts it back to where it starts.
- The height is remembered: closing and reopening the editor, or opening another project, keeps it,
  the same way the Scene list's and the Inspector's widths are kept.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Move the pointer onto the border between the Scene list and the Assets panel.
3. Press and drag it up or down. Nothing moves and the border does not light up.
