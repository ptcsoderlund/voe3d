# 0390 — A probe inside geometry renormalises the read and never darkens it
date: 2026-10-08
by: tech-lead

## Decision
For 082 bug 05, amending 0389 point 6. A volume's weight a counts only the grid's edge fade and, when readiness
counts, the readiness of the valid probes: a = edge fade × (Σ trilinear × validity × readiness) / (Σ trilinear ×
validity), and 1 in place of the fraction when readiness does not count. Validity, facing and visibility still
choose which probes the rgb is normalised over, but a probe inside or behind geometry never lowers a. So the
ground beside a box takes the full bounce of the probes that can see it, in the level grid and in every nest,
and in the relight's own read. a is 0 when no probe is valid or the sum is not finite. The lit-side check in the
3d bounce scene keeps its assertion (051 step 3): the ground beside the lit face is clearly more coloured than
ground further out. The test places its eye where the 1 m nest covers the box, since 0387 makes small things'
colour a near-camera detail. Two choices made while carrying out 0389 are accepted as built: a nest goes home
when its move would be a whole grid or more, measured by the move and not by the eye's offset (card 48); and a
new caster is one never remembered in a world where some transform has been remembered, because scene has no
query for its previous table (card 52), a gap of the first frame only, since editor and game remember every
frame.

## Reasoning
With validity inside a, a box's own probes, which sit inside it and are invalid, pulled the read down right
where its colour should be strongest: card 56 measured 4/255 beside the lit face against 6/255 1.6 m out.
Invalid probes are a fact of the scene, not of the volume being unready, so they belong in the normalisation
only; a exists to fade a grid at its edges and while its pictures arrive.
- Loosen the lit-side check: green today, but it gives up on bug 05 itself.
- Split bug 05 into its own work order: the same work, and the hill is not fine without it.
- Leave a as it is and lean on the finer nests: the darkening comes back in every nest beside every object.

## Replaces
Nothing. Amends 0389 point 6; confirms the card 48 and 52 readings of 0389 points 2 and 7.
