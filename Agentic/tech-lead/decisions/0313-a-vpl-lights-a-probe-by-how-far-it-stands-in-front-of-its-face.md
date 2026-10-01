# 0313 — A VPL lights a probe by how far the probe stands in front of its face
date: 2026-10-01
by: planner

## Decision
For 046 bug 03, the cheap means 0312 leaves to the planner. In `bounce.slang`'s gather, a VPL is measured from
its own position (0309's move is gone) and its term toward a probe is multiplied by
w = saturate(n · (probe − VPL) / spacing), n its unit normal: nothing for a probe on or behind its face's
plane, rising to whole one cell (2 m) in front. The flux, the cosine, the 1 m² floor and the 1 mm skip are
unchanged. `VOE_BOUNCE_GAIN` is chosen again under 0311's rule, now with 0312's shadow-side bound as a second
condition: the lowest whole number at which the tank captures meet both.

## Reasoning
Two paths carried a face's colour behind it. 0309 placed every VPL 1 m behind its face, so a box 1 m deep (the
tank game's obstacles) shone from its back face into its own shadow. And a receiver reads the eight probes up
to one cell around it: one 0.5 m in front of a face hands that face's light to ground 0.5 m behind it, which
the raised gain made plain. Weighting by the probe's standoff bounds what any receiver behind a face can get
from it to a quarter of a probe's full term, and nothing from a probe on the plane; it costs one dot product
in a loop that already takes one. A toy model of the tank's Obstacle kept its shadow-side foot unchanged at
gains up to 20 while the lit side still faded from 1 m to 8 m.
- Withdraw 0309 alone: in the toy model the shadow foot still rose 3 to 16/255 at gains 10 to 20, through
  the interpolation.
- A hard cut at half a cell: the same bound, but a face sliding across it pops the probes.
- Per-probe depth or visibility (DDGI's moments): needs rays 0307 rules out.

## Replaces
Nothing. Carries out 0312; amends 0308 point 4 and 0311's measure.
