# Needs decision: how a small thing colours the ground on a large level

From bug 05 (`bugs/05-a-small-coloured-box-does-not-tint-the-ground-beside-it.md`).

## The question
The probe bounce cannot show a 1 m box's colour on the ground, and on a 1 km landscape no bounce from
anything smaller than tens of metres can show. Do we amend 0331 now, and with what?

## Why it is not a code bug
- 0332 fits a fixed 24 × 12 × 24 grid to the still casters' box at 2 m × 2^k. A 1 km landscape needs
  k = 5: probes 64 m apart, 8-texel faces. A 1 m cube is less than a texel of any probe's picture.
  0331 says so in its own words: "a larger level gets the same grid stretched coarser … finer bounce
  near the camera waits for a LOD system".
- Even at 2 m spacing (the orange floor, if it is under about 44 m) a 1 m box is below the horizon of
  the lowest probe layer (1 m up), so the ground's upward irradiance (0327's +y axis) barely sees it.
  `3d/tests/bounce_scene.c` proves the tint with a 2 m box on a 40 m ground; that passes and stays true.
- So 051's step 3 holds for boxes the size of a cell on a level the size of the tank game, and fails
  for the hill's world, which 0374 makes the 0.3 target. Changing that changes how the engine bounces
  in every game: it is beyond 082 and against 0331 as written.

## Options
1. **A cheap local bleed from small lit things (recommended).** Each still caster smaller than a few
   grid cells adds, in lit shading, a one-bounce term from its sunlit faces: colour = its albedo × the
   light it receives × bounce strength, onto surfaces in front of each lit face, fading over about its
   own size. World-fixed and settled like the grid (0328, 0331), costs a read only, works at any level
   size, keeps 0312 (only out of the lit side). One bounce, no visibility past the thing's own faces,
   so its reach must stay short. Fits 0307's "cheap cheats over physics".
2. **Fine local probe volumes on small still casters.** A small 0.5–1 m grid fixed to each such thing's
   box, read over the level grid. Same machinery as 051, sees walls, but memory and capture grow with
   the number of things; a hill with hundreds needs a cap and a choice of which get one.
3. **Keep 0331 and wait for the LOD system.** No work now; the hill ships with no small-thing tint,
   which the sponsor's bug says is not acceptable for 0.3.

## Also in the bug
`05-purple-cube-on-orange-floor.png` shows a faint curved edge across the floor with nothing to cast
it, against 051 step 2. A curve suggests a sphere: the relight sun map's sphere at the volume's reach
(0332 point 4), a sun cascade's edge, or the grid's outer-cell fade. It can be planned as its own card
once the floor's size is known; it does not need this decision, but the bug is one report and is
planned whole once the answer lands.
