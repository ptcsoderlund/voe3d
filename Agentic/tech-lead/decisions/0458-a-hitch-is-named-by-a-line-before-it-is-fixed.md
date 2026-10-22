# 0458 — A hitch is named by a line before it is fixed
date: 2026-10-22
by: planner

## Decision
For 090 bug 08 (the editor stutters in lumberhill while the sun bounces).

1. **`app/hitches.h` finds a hitch against the frame's own mean.** Two pure calls, one for the
   frame's wall interval with the program's named CPU stretches, one for the card's measured
   total with render's pass times. Each keeps an exponential mean of weight 1/60; none is a
   hitch before 60 samples; a sample above 1.5 × the mean and above 1/60 s is one. Every sample
   is folded into its mean, a hitch too.
2. **A hitch writes one line, its five costliest parts.** The frame's line names its interval,
   the mean and its five longest stretches; the card's its total, the mean and its five costliest
   pass names, passes of one name summed with their count. Milliseconds, one decimal.
3. **The editor watches every frame.** Its loop marks its stretches (frame open, step, views,
   interface, end), times its shadows calls apart, and puts each hitch line on stderr after
   `editor: hitch: `. Always on: a line costs nothing until a frame stalls.
4. **The fix is planned from the lines.** Bug 08's cards are the measurement; the sponsor runs
   lumberhill with them and files the lines as the bug its fix is planned from.

## Reasoning
- The bug names four bounce works that run now and then and says which one stalls is not known;
  0358 argues renderer performance from a measurement, and a fix of the wrong one costs a card
  and leaves the stall.
- The Frame panel copies one frame four times a second, so a single bad frame slips past it; a
  line written by the stalled frame itself cannot.
- Wall time and card time apart, because the stall may be on the CPU (a volume built at the top
  of a frame idles the GPU, the bounce's caster walk) or on the card (capture passes, sun maps
  that now draw the foliage, a whole relight); the line says which.
- In `app`, not the editor: it is a frame loop's part, testable without a card or window, and a
  game may watch itself the same way later; the editor has no tests folder.

## Replaces
Nothing.
