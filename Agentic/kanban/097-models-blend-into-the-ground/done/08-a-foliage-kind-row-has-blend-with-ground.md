# 08 — A foliage kind row has Blend with ground
folder: editor
after: 07
decisions: 0168, 0481, 0473, 0476

## Change
0481 point 4 on screen: on each kind's row of the Inspector's Foliage section, after its Cast
shadows button, a "Blend with ground" button and a "Blend height cm" slider with its figure,
each editing the kind's look through the existing look ask, as Cast shadows and the spreads do.

- `editor/src/sculpt.h` — beside the Cast shadows nodes: per kind a Blend with ground node and a
  blend height slider with its figure (VOE_ASSETS_LANDSCAPE_KINDS each), the slider's range
  `VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN` to `_MAX` (assets/landscape.h), whole centimetres, a
  look's 0 shown as `_DEFAULT`. The comment on the Foliage section's nodes follows.
- `editor/src/sculpt.c` — `look_same` compares `blends` and `blend_height` too, so a blend
  change is an undo step.
- `editor/src/inspector_foliage.c` —
  - Draw: after each kind's Cast shadows, the button drawn chosen while the look `blends`, then
    the slider and figure, drawn whether or not it blends.
  - Forget: the new nodes forgotten with the others.
  - Read: a fired button asks the look with `blends` flipped, not held, one step at once; a
    changed slider asks it with `blend_height` changed, held while the slider is held.
- `editor/src/inspector_foliage.h` — the first paragraph's list of a kind row's controls; a
  sentence that the blend is the kind's look (0481 point 4), off by default, each change one
  undo step, and that its cost shows as `foliage blend` in the frame breakdown.
- `editor/src/interface.h` — if its paragraph counting the Inspector's nodes counts the Foliage
  section, it grows by the new nodes at sixteen kinds.
- `editor/src/src.md` — the `inspector_foliage.*` and `sculpt.*` entries name the new controls.

## Done when
The editor builds, and `grep -n "Blend with ground" editor/src/inspector_foliage.c` prints a line.
