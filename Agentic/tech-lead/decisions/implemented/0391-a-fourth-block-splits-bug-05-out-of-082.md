# 0391 — 082 bug 05 gets one more replan; a fourth block splits it out
date: 2026-10-08
by: tech-lead

## Decision
For 082 bug 05, answering its needs-decision after three replans: the plan goes on. Card 63 found the fault
outside 3d: any whole-grid bounce relight leaves the probes stale (bounce strength 1 → 2 → 1 settles to a
different patch), with no blocker involved. The next card is in render: repeat it in `render/tests/blocked_bounce.c`
with the eye off the origin, nest cell (−12,−4,−4) and an incremental recapture before a lights change, then fix
the whole path of `voe_render_bounce_relight`. 0387, 0389 and 0390 stand unchanged. If the feature blocks again
after this replan, bug 05 is cut out of 082 into a work order of its own rather than replanned a fifth time.

## Reasoning
The three blocks each moved one step closer to a single, reproducible fault in a folder the cards may touch, so a
replan aimed at it is likely to hold.
- Split bug 05 out now and accept the hill without the tint: cleaner, but 0387 ruled out shipping without it.
- Settle for the coarse bounce: cheapest, but reverses 0387.

## Replaces
Nothing.
