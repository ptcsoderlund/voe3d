# 0332 — The bounce grid is fitted to the still casters' box, at a power-of-two spacing
date: 2026-10-03
by: planner

## Decision
For 051 bug 03, carrying out 0331 and replacing 0329 point 1:
1. **The box.** 3d takes the world box, in double, of every caster the stale walk visits that did not
   move this step (lag 1 equals lag 0): each geometry's own vertex box, which render keeps at create
   and hands back by `voe_render_geometry_box`, under the caster's world transform at lag 0 from
   `voe_scene_transform_between`, never through the eye-relative matrix. Casters in motion (a shell, a
   driving tank, a box being dragged) do not stretch it; they read the grid where they are. No still
   caster: the grid at the finest spacing about the world origin.
2. **The fit.** Still 24 × 12 × 24 probes (0318: memory and relight cost fixed). The spacing is
   `VOE_RENDER_BOUNCE_SPACING` (2 m) × 2^k, the smallest k at which the grid whose lowest cell is the
   box centre's cell less 12, 6 and 12 holds every cell of the box with one cell to spare on each
   side, so the read's fade over the outer cell falls outside the level. Cells are whole cells of
   that spacing about the world origin. The eye is used only for `corner`. The tank game's 40 m
   ground fits at 2 m.
3. **Render places by it.** `voe_render_bounce_frame` carries the spacing; every record and capture
   uses the volume's. A spacing other than the last one empties the whole grid, as a jump does. A
   box whose centre crosses a cell scrolls the grid one cell, capturing the slab it brings in.
4. **The reach scales.** `VOE_RENDER_BOUNCE_REACH` (24 m) is the reach at the finest spacing; a
   volume's reach is 12 of its cells, so a coarser grid's probes still see past their neighbours.
   The relight's sun map sphere uses the volume's reach. A moved caster's stale sphere is the larger
   of `VOE_3D_BOUNCE_REACH` and 3 cells.
5. **No camera in the relight.** The relight-needed test compares lamp positions about the grid's
   lowest corner within a millimetre, and a lamp's shadow by whether it is slotted and its strength,
   not by which slot, so an eye that moves never relights.

## Reasoning
A box from the still casters answers 0331 with no state across frames: the same world gives the same
grid in every view and the game. Powers of two make a spacing change, which recaptures 6912 probes
(about 108 full frames), rare while editing. A shell flying off the ground would have doubled the
spacing if moving casters counted. The eye-relative float matrix would let a camera move flip a
floor at a cell edge, a scroll the eye causes. Known limit: past 16 casting lamps, 0325's per-eye
shadow fade still reaches a bouncing lamp's relight.
- Spacing fitted exactly per axis: shaders take one spacing; every small edit would recapture all.
- The bounding sphere render already keeps: a 40 m flat ground becomes a 70 m cube, 8 m spacing.
- Keeping the last grid until the box leaves it: needs per-target state in 3d, and views disagree.

## Replaces
Replaces 0329 point 1 (placement by the eye) and amends 0326 points 2 and 4 (spacing, reach, stale
sphere). 0329's sun map stays, fitted to the grid.
