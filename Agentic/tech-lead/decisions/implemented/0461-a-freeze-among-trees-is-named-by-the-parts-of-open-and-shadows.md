# 0461 — A freeze among trees is named by the parts of open and shadows
date: 2026-10-22
by: planner

## Decision
For 090 bug 11 (the editor freezes for minutes when the camera moves among trees), under 0458.

1. **The editor's `open` stretch splits by call.** `open` keeps the theme, a refresh, saves and the
   history step; `world` is the world step; `undo` the undo settle; `pace` is `voe_app_frame_open`,
   whose wait is 0215's quarter second when the window is not focused and nought when it is. The
   draw's open, the slot's fence wait, leaves `views` as `fence`.
2. **The shadows call times its parts when the caller hands it a clock.** `voe_3d_frame` gains a
   `double (*clock)(void)`, NULL by default, and five summed seconds: `casters` (shapes and models
   into the cascades), `ground` (a landscape's nodes into them), `foliage` (the groups into them),
   `lamps` (the point-shadow pass) and `bounce` (the probe bounce). With no clock nothing is read.
3. **The frame line lists them.** The editor sums each part over its views as `shadows: <part>`,
   times the foliage gather as `gather`, and holds up to 16 stretches a frame.
4. **The card line ends with the frame's triangles and draws**, render's counts of the newest
   recorded frame, so a foliage span is told triangle-bound from fill-bound.
5. **The fix is planned from the new lines** (0458 point 4). Until `pace` says otherwise, the steady
   249.5 ms frames are read as 0215's unfocused wait. If the new card lines show the forest's
   triangles filling the card, the next plan asks for 095's culling in a `needs-decision.md`.

## Reasoning
- Bug 11's lines say `shadows` took a second with bounces at 0, but the call holds four kinds of
  work and the headers name none that grows near trees; a fix of the guessed one costs a card and
  leaves the freeze, which is what 0458 was written against.
- 249.5 ms is the editor's unfocused heartbeat to the tenth of a millisecond; a stretch of its own
  proves or rules that out in one run, and the one 2.6 s `open` is then pinned to a call.
- A clock handed in rather than read: 3d does not name platform, and a new edge is a decision
  beyond this feature. A function pointer in the frame is one field, unused by the game and dev.
- The card's foliage span is either too many triangles (no culling draws every copy at its level,
  behind the camera too) or too many cut-out pixels near the eye; 095 helps only the first, so the
  count decides before 095 is asked for.

## Replaces
Nothing. Extends 0458 points 2 and 3.
