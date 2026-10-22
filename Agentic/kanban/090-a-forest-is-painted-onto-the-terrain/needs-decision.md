# Needs decision — bug 12: is the forest's card time 090's, or 095's?

## Question
Bug 12's card lines (as bug 11's) put every card hitch in `view target 1: foliage`, 40–104 ms, at
5.65–5.74 million triangles in 640 draws, with a card mean of 27–30 ms. 0461 point 5 says: if the
forest's triangles fill the card, the next plan asks for 095. They do: every copy is drawn at its level,
behind the camera too, and the count holds near 5.7 M whatever the camera faces. But the count is flat
while the foliage time swings 40–104 ms, so the spikes on top of that floor track something 095 does not
cut, most likely cut-out leaf pixels near the eye. Bug 12 says the planner may not pull 095 in.

## Options
1. **090 accepts the forest's card time; 095 moves up to come right after 090** (ahead of 091–094). The
   mean, the larger share, falls with 095. The swings are measured again once 095 is in.
2. **095 is pulled into 090** as its last cards. The forest is then smooth when 090 is accepted, but 090
   gets a work order's worth of cards across render, 3d, editor and game, and the swings stay.
3. **090 cuts its own foliage cost**, without 095: a per-kind draw distance past which copies are not drawn.
   This is a new authored setting the sponsor did not ask for, and 095 still comes later.

## Recommendation
Option 1. Bug 12's foliage part is closed by that decision, with no 090 card.

## The rest of bug 12, for the plan once this is answered
These parts need no decision. They were not planned because the planner writes no cards while a
question is open.
- **A frame's stretches do not add up because the frame is timed over a different span than its
  stretches.** `voe_editor_frame_hitches_end` gets `opened.tick.elapsed`, and `voe_app_frame_open` reads
  its clock after the wait and before the poll (app.h). So the tick runs from one read to the next, while
  the watch's stretches run from `_begin` to `_end`. The poll, and everything after `pace`, are counted
  in the next frame's `elapsed`. That explains both a `pace` 8002 ms inside 4066 ms (a long poll) and
  13 s unnamed. Fix: the watch times its own frame from `_begin`'s reading to `_end`'s (one editor card).
  Also, `voe_app_frame` gains `double waited`, the seconds of the 0215 wait (one app card first), which
  the watch lists as `pace: wait` within `pace`. That tells a hidden or unfocused wait from a poll that
  stalls.
- **`shadows: bounce` names a function, not a cause.** No header names CPU work that could take a
  second. `voe_3d_draw_bounce` does the following per volume, four volumes:
  - the still casters' box and fit;
  - the stale and removed spheres' walks;
  - `voe_render_bounce_begin`'s bookkeeping (nearest-first take over 6912 probes);
  - up to four capture passes with every caster;
  - the sun maps with foliage;
  - the relight.

  Under 0458 point 4 the next step is to time those as parts of `voe_3d_shadow_times`, with
  `frame->clock`, in `3d/src/draw_bounce.c` (one 3d card), listed by the editor as `bounce: <part>`
  (folded into the editor card above). Then the stalled part is fixed under 0386.
- **The frame line does not say whether bounces were at 0.** `shadows: bounce` appears only when a
  light bounces. The editor card's header says so.
