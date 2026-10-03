# 0327 — A probe's bounce is held as irradiance along the six axes, not L1 SH
date: 2026-10-02
by: planner

## Decision
For 051, amending 0326 points 6 and 7. Each level grid and the sum hold, per probe, the irradiance along
+x, −x, +y, −y, +z and −z (an ambient cube), RGB, in the same three RGBA16F 3D images a grid, now
48 × 12 × 24: image a is axis a (x, y, z), probe (i, j, k) at texels (2i, j, k) for the positive direction
and (2i + 1, j, k) for the negative, alpha unused. The relight sums each texel's radiance × solid angle ×
max(0, ±direction_a) into the six, which is the exact clamped-cosine irradiance along each axis. The read
takes E(n) of a probe as Σ_a n_a² × E(sign(n_a) axis a); the rest of 0326 point 7 stands. Nothing else
changes: the bindings, the image counts, the calls.

## Reasoning
With L1 SH a probe above sunlit flat ground gives an upward normal light from the ground ring below its
horizon: card 12 measured 23 of the lit ground's 94 in a box's shadow at its foot with bounces 1 against 0
at 0, breaking 0312 and step 3 of How to test. Along an axis the six-axis irradiance is exact, so flat
ground and walls take nothing from behind their own plane, and on flat ground every probe still reads the
same. 1.2 MB more a target than L1 SH (2.3 MB of grids).
- L2 SH: still rings, and 27 values a probe against 18.
- Non-linear L1 reconstruction: sharpens a lobe but still lights an upward normal from a low ring.
- Lift the bound or a fill floor in the test: hides a leak the sponsor would see as washed-out shadow.

## Replaces
Nothing. Amends 0326 points 6 and 7 (L1 SH → six-axis irradiance).
