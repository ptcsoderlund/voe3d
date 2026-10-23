# 08 — Each foliage kind row has its own settings
folder: editor
after: 07
decisions: 0168, 0469, 0473, 0476

## Change
0473 point 2 and 0476 point 1 on screen: under each kind's row in the Inspector's Foliage
section, after its Base size slider, a Cast shadows button and the Size ±%, Turn ±° and Tilt with
slope % sliders, each editing that kind's look through card 07's look ask.

- `editor/src/sculpt.h` — beside `kind_sliders` and `kind_figures`: per kind a Cast shadows
  node and three spread sliders with their figures (VOE_ASSETS_LANDSCAPE_KINDS each), and the
  ranges in the look's units: Size ±% 0–100, Turn ±° 0–180, Tilt with slope % 0–100, whole
  numbers. The comment on the Foliage section's nodes follows.
- `editor/src/inspector_foliage.c` —
  - Draw: for each kind, after its Base size slider, a "Cast shadows" button drawn chosen while
    the kind's look is not `shadowless`, then the three sliders with their figures, the labels
    as the brush's were.
  - Forget: the new nodes forgotten with the others.
  - Read: a fired Cast shadows button asks the kind's look with `shadowless` flipped, not held,
    so it is one step at once; a changed spread slider asks the look with that spread changed,
    held while the slider is held, as the Base size slider does.
- `editor/src/inspector_foliage.h` — the header's first paragraph lists each kind row's
  controls; a paragraph says the spreads and Cast shadows are the kind's (0476 point 1), each
  change one undo step, and that Cast shadows off leaves its copies shadowed.
- `editor/src/interface.h` — if its paragraph counting the Inspector's nodes counts the Foliage
  section, it grows by the new nodes at sixteen kinds.
- `editor/src/src.md` — the `inspector_foliage.*` entries name the new controls.

## Done when
The editor builds, and `grep -n "Cast shadows" editor/src/inspector_foliage.c` prints a line.

Human (How to test 6): on the hill, turn Cast shadows off on the fern kind: ferns stop casting
but are still shadowed; on again: they cast. Ctrl+Z puts it back.
