# 0476 — Grown foliage is made from the paint, and each kind carries its spread
date: 2026-10-23
by: planner

## Decision
For 095, carrying out 0473 as 0475 amends it.

1. **A kind's look.** Each foliage kind carries its base size (0469), Size ±% (0–100, default 20),
   Turn ±° (0–180, default 180), Tilt with slope % (0–100, default 0) and Cast shadows (default on).
   The three spreads move from the brush to the kind, saved in `[Foliage]` as `vary<k>=`,
   `turn<k>=`, `tilt<k>=` and `shadow<k>=0` (written only when off), a missing key the default.
   Density stays on the hand brush. A hand-painted copy draws its scale, turn and lean from its
   kind's spreads when painted and keeps them; a grown copy draws them from its kind's at each
   remake.
2. **The choice is on the landscape's layer row**, `[Layers]` `grow<l>_<k>=` in tenths of copies
   per 100 m² (1–500, so 0.1–50), written only above 0. A row given another material keeps its
   row's choices.
3. **Grown copies are not stored; they are made from the paint.** Kind k's points lie on a
   jittered lattice from the grid's corner at spacing √(1000 / D) m, D the most tenths any layer
   gives k; a cell's point and draws come from a hash of (k, cell) and no state. The point is kept
   where a draw falls under d / D, d the tenths at the point: each layer's bilinear weight / 255
   times its tenths, summed. So a remake over any area gives what a remake of the whole grid
   gives there, and the editor, Play and a shipped game grow the same copies from the same file.
   A copy belongs to an area by its position, half-open.
4. **When they are remade.** All at a load; every kind over the area a paint stamp, a sculpt stamp
   or an undo's put changes, widened by one cell; one kind over the whole grid when a layer's
   density of it or its spreads or base size change. The game grows them from the cooked choices
   at load, under the splash.
5. **The store keeps them beside the file's copies**, room VOE_ASSETS_LANDSCAPE_COPIES per
   landscape held at load. Scatter, Erase and undo's copy puts touch only the file's copies.
   Past the room a remake adds what fits and says so once on stderr; which copies stand may then
   depend on the order of edits.
6. **Cast shadows off** draws the kind's copies in the camera pass only: in no cascade and no
   bounce sun map. They are still shadowed.
7. **Each setting and each tick is one undo step**, as a base size is (0469).

## Reasoning
- The choice in the `.material` file: a material is shared by landscapes whose kind lists
  differ, and a kind is an index into one landscape's list.
- Storing grown copies in `[Copies]` with a flag: every repaint would add and remove file lines,
  the file and the cook would grow with every tuft, and keeping them in step with the paint
  is what remaking from the paint gives for free.
- A seeded scatter like the brush's at each repaint: copies outside the painted area would
  move each time, and the game could not make the editor's copies again.
- Spreads left on the brush: grown copies need them per kind (0473 point 2), and one place for
  them is less to learn.

## Replaces
Nothing. Amends 0442 point 3 and 0469 point 2: Size ±%, Turn ±° and Tilt with slope % are each
kind's, not the brush's; the brush keeps Density.
