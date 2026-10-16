# 0437 — Strength goes, and the LOD rows have column headers and a swap line
date: 2026-10-16
by: planner

## Decision
For 089 bug 09. The sponsor removes Strength, which changes 0435 point 1; the planner's choices follow.

1. **Strength goes everywhere.** `voe_assets_levels` loses `strength`, and the
   `voe_assets_levels_strength` enum and its three words go. The cook's parts table writes no strength.
   "LODs of <file>" has no Strength row.
2. **Fixed at Medium's values.** The simplifier always lets normals within 60° share a side and lets a
   collapse cost at most a quarter of the model's bounding radius. These are two named constants in
   `3d`, not a setting. A model made with the defaults is made exactly as Medium made it.
3. **Old `.parts` files.** A `strength` key in `[Levels]` is read and ignored, whatever its value; a
   save never writes it. Any other unknown key stays MALFORMED.
4. **No new levels file version.** Version 5 stands. A file made earlier under Gentle or Strong stays
   fresh until the model's next Make LODs that changes a count or share, or a change to its `.glb`, as
   0430 point 7 already says of a hand-edited `.parts`. A Make that changes only swaps still reloads
   without preparing.
5. **The rows' headers.** Above the first LOD row is a header row: nothing over the names,
   "Triangles kept" over the share, "Swap distance" over the swap, "Triangles" over the count. Each of
   the four columns has one fixed width in millimetres, shared by the header row and every LOD row, so a
   header sits over its values. Own levels have the same headers, their share being a label.
6. **The swap line.** Under the LOD rows is one line in the secondary text role. It says that ×2 swaps
   the LOD in twice as far away and looks better, and ×0.5 twice as close and saves triangles. With one
   LOD there are no rows, and so no header row and no line.

## Reasoning
The per-LOD shares made Strength's cut meaningless, and what it still set did not show up as anything a
person could judge in the panel. Fixing it at Medium keeps the stable's LODs as they are now. Bumping the
file version would re-prepare every model of every project to change only the rare Gentle or Strong one,
which the next Make does anyway. Fixed column widths line the headers up without reshaping the rows into
columns. Reading an old key and dropping it costs one line and keeps every saved `.parts` opening.

## Replaces
Amends 0435 point 1: Strength no longer exists, and its edges and collapse limit are Medium's, fixed.
Amends 0430 points 2, 4 and 6 and 0436 points 1 and 8 where they name strength.
