# 0314 — A VPL reaches a probe just behind its face
date: 2026-10-01
by: planner

## Decision
For 046 bug 03, amending 0313's weight after card 27 blocked. In `bounce.slang`'s gather a VPL's term toward a
probe is multiplied by w = saturate((n · (probe − VPL) + o) / spacing): 0313's standoff plus an allowance o, so
a probe up to o behind a lit face's plane still takes a share of its light, rising to whole one cell less o in
front. o is `VOE_BOUNCE_STANDOFF`, a constant in `bounce.slang`, the smallest of 0.25, 0.5, 0.75 and 1 m at
which a whole `VOE_BOUNCE_GAIN` up to 50 makes the tank captures meet 0310's tint (blue at 1 m at least blue at
8 m + 12, 4 m between them) and 0312's shadow-side bound at sun 1 and π, with render's SHADOW SIDE still
passing; the gain is the lowest such. Everything else in 0313 stands. No o up to 1 m doing that is a question
for the tech lead on 0310's fade or 0312's margin.

## Reasoning
Card 27 measured 0313 on the RTX 4070: the shadow side held, but the ground 1 m from the tank Obstacle's lit
face read darker than 4 m out at every gain, because the probe that serves it sits 0.09 m behind that face and
0313 gives it nothing. A probe a little behind a face is inside the caster for any caster thicker than o, and a
receiver beyond the far face reads it only when the caster is thinner than one cell plus o less the receiver's
distance, so the leak is bounded and SHADOW SIDE's 1 m wall measures it. The smallest o that works keeps that
bound tightest.
- Weight per receiver: the grid holds no per-VPL record; that is per-probe depth, which 0307 rules out.
- A 1 m grid: halves the 64 m 0308 covers.
- Drop "4 m between" or widen the 2/255: 0310 and 0312 are the tech lead's.

## Replaces
0313's w. Amends 0313 and 0311's measure.
