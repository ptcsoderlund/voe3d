# 0440 — A scene-wide row is the first whose entity is on
date: 2026-10-16
by: planner

## Decision
For 089 bug 10, carrying out 0427 for the Sky, Haze and Picture rows.
1. The row a pass takes from each of the three tables is the first in table order whose entity
   `voe_scene_identity_off` (0428 point 2) says is on; an off row is passed over as if it were not in
   the table. With none on, the pass has what it has with no row: no sky, no haze, x1 and Clip.
2. The test is made where the rows are read, in `3d`'s `voe_3d_draw_system_sky` and
   `voe_3d_draw_system_picture`, so the game's frame, Play, a shipped game and the editor's views,
   which all take the rows through those two, follow it at once. `scene` keeps every row; nothing is
   removed or copied.
3. Each row type is asked of its own entity, so two rows on one entity go together, and a row under an
   off parent is off, as 0428 point 2's read already answers.

## Reasoning
"As if it were not there" means the next row is the one used, not that an off first row blanks the
table; the capacity is one in the game, so the difference only shows where a caller allows more. One
read at the one place the rows become a pass's record keeps the rule in one place, as 0428 did for
draws.

## Replaces
nothing; amends the "first row in table order" sentence of 0405, 0406 and 0416 as they stand in
`scene`'s headers.
