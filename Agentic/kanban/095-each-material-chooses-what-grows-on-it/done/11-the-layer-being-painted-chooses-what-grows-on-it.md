# 11 — The layer being painted chooses what grows on it
folder: editor
after: 10
decisions: 0168, 0402, 0473, 0476
read: feature.md

## Change
0473 point 3 on screen: beneath the Layers section's row of the layer chosen to paint, one row
per foliage kind of the grid, ticked where that layer grows it, and for each ticked kind a
Density slider. Each asks card 10's grow edit.

- `editor/src/sculpt.h` — the grow rows' nodes: per kind a tick button and a Density slider with
  its figure (VOE_ASSETS_LANDSCAPE_KINDS each), forgotten each frame as the layer rows are.
- `editor/src/inspector_layers.c` —
  - Draw: while PAINT is chosen, under the chosen layer's row and indented, a "Grows" label,
    then per kind a button by its model's file name drawn chosen while the layer's density of it
    is above 0, followed for a ticked one by a "Density" slider in copies per 100 m², 0.1–50,
    its figure to one decimal. A grid with no kinds shows "Add a foliage kind below" instead.
  - Read: a fired tick asks density 0 when ticked, else the default (card 10), not held; a
    changed slider asks its tenths, held while the slider is held.
  - Forget: the new nodes too.
- `editor/src/inspector_layers.h` — the header's first paragraph says the chosen layer's grow
  rows; a paragraph says the choice is the layer row's and saved with it (0476 point 2), that
  the material's kinds grow by themselves where it is painted, thick where the paint is strong,
  and that each tick and density is one undo step.
- `editor/src/interface.h` — if its paragraph counting the Inspector's nodes counts the Layers
  section, it grows by the grow rows at sixteen kinds.
- `editor/src/src.md` — the `inspector_layers.*` entries name the grow rows.

## Done when
The editor builds, and `grep -n "Grows" editor/src/inspector_layers.c` prints a line.

Human (How to test 1–8): open the painted hill; add a foliage kind whose model is a grass tuft;
choose the grass layer and tick the tuft: tufts grow where grass is painted, none on dirt, moss
or rock. Untick: they go at once; tick: they come back. Choose moss, tick the tuft at a lower
density: thinner tufts there. Tick a fern on moss: ferns on moss only; paint moss into a new
patch: ferns and tufts grow as you paint. Erase over grown ferns: they stay; over hand-painted
ferns: those go. Paint ferns by hand onto dirt: they stand. Save, reopen, Play and Ship: the same
foliage in each. Fly low over thick tufts in a release build on the laptop: smooth.
