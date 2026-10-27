# 09 — A shipped game keeps each kind's blend
folder: editor
after: 03, 08
decisions: 0168, 0481

## Change
Play and Ship cook each landscape's kind looks into C; the cooked look carries the blend too, so
a shipped game's foliage blends as the editor's (0481 point 1). A model row's two fields are
described and cooked with no work here.

- `editor/src/game_tree_landscapes.c` — each cooked look line is `{size, size_vary, turn_vary,
  tilt, shadowless, blends, blend_height}` in the struct's field order; the header's example
  `landscape_0_looks` line and the bytes it budgets a look's cooked line follow, if its width
  grows past them.
- `editor/src/src.md` — the `game_tree_landscapes.*` entry says the looks carry the blend.

## Done when
The editor builds, and `grep -n "blend_height" editor/src/game_tree_landscapes.c` prints a line.

Human (How to test, on the hill):
1. Place a rock half into the painted hill, Blend With Ground off: the seam is as before.
2. Turn it on: the foot takes on the painted layer under it and fades into stone higher up, no
   line where it enters the ground, in sun and in shade.
3. Raise and lower Blend Height: the fade reaches further up and back.
4. Drag it onto another layer, repaint under it, sculpt up around it: the blend follows at once.
5. The house's concrete base left off keeps its hard edge.
6. In the Inspector's Foliage section turn Blend with ground on for a tree kind: the trunks'
   feet blend; fly low through the forest, it stays smooth, and the frame breakdown lists
   `ground blend` and `foliage blend`.
7. Save, close, reopen: the switches and heights are as left.
8. Play, then Ship and run the shipped build: the blend looks as in the editor's view.
