# 106 — needs decision

0495 leaves three things open that the feature's tests depend on. Precog marks all three Open, so the
planner does not choose them. No cards are written until they are settled. Effort is left out of every
recommendation (0483).

What the code does today, for context. Every bound is made in one place, the store's
`3d/src/models_levels.c`: 0.002 × radius ÷ error ÷ swap, made non-increasing, or 2^-k ÷ swap for own
levels with no fresh file. The cooked file carries errors and swaps, not bounds, so one spreading there
covers load, cook and Play. Foliage dissolves as well as model rows do (`rest` groups, `3d/src/draw_foliage.h`).
So the 0432 band is spatial in every at-once draw, and only shadows and held parts draw whole.

## 1. The dissolve when a copy swaps back (test 2)
Going out, a copy takes LOD k at b_k and the 0432 band runs from b_k down to 0.8·b_k. It swaps back
only at b_k/0.9, which is outside that band.

- **A. The picture stays a function of distance.** The band stays where 0432 put it and is the same in
  both directions. The kept LOD decides what is drawn whole (shadows, held parts, foliage casts), the
  Inspector, and which pair dithers. Creeping back inside the band dissolves the dithered parts partly
  back towards the finer LOD, while the Inspector still says the coarser one.
- **B. The dissolve runs over time at each swap.** Each copy per view keeps its LOD and the moment it
  swapped. Both LODs dissolve over a fixed time, for example 0.25 s, at whatever distance the swap
  happened. Creeping back shows the coarser LOD whole. Coming 10% nearer swaps back and dissolves
  then. A copy standing still never stays stippled (0432 had to accept that stipple). The 1.25 gap
  still makes every LOD show over a stretch of distance.
- **C. The band follows the direction of travel.** Each copy also keeps how far its dissolve has got,
  and that only moves forward until the next swap. This needs more state than A or B, and the
  picture depends on the path the copy took.

Recommendation: **B**. It is the only option where all of test 2 shows on screen ("keeps the coarser
LOD", then "swaps back, dissolving"), and it removes the stipple on a copy at rest. It replaces 0432
point 3's band in `s` with a dissolve over time, and 0495 point 1's reason for 1.25 becomes "a stretch
per LOD", not "one band apart".

## 2. What "Swap distance" shows, and whether spread values are saved (test 1)
A multiplier scales that LOD's own error, so two rows' multipliers cannot be compared. Only a distance
can show "farther by at least a quarter".

- **A. Metres.** The distance the LOD swaps in at for the model at scale 1, under the default lens of a
  scene camera. Made LODs do not depend on the radius: 500 × error × swap ÷ tan(half fov). The × box
  stays as the input, under its own "Swap" header. "—" when there is no fresh file.
- **B. Relative to LOD 2.** LOD 2 shows ×1.00, and each later row shows how many times farther it swaps
  in (≥ 1.25 after spreading). This depends on no lens and no scale, but is less concrete than metres.

In both, the spread values are applied when read and never written back to `.parts` or the levels
file. The typed multipliers stay as typed, which is what makes test 6 hold for old projects.
Recommendation: **A**, with "applied when read".

## 3. A LOD with no error, and own LODs on the 2^-k fallback
Error nought gives an infinite bound. That LOD is always allowed, so every LOD before it never shows
except with the eye inside the sphere. This goes against 0495 point 1.

- **A.** Keep "always allowed". Spreading starts after the last infinite bound, and the panel shows the
  hidden LODs as never used.
- **B.** Cap every bound at s = 1 (the radius fills half the screen) before spreading. A lossless LOD
  then swaps in once the model no longer fills the screen, and the LODs before it each get their own
  stretch nearer than that.

Own LODs on the 2^-k fallback are finite bounds and are spread like any other in both options.
Recommendation: **B**.
