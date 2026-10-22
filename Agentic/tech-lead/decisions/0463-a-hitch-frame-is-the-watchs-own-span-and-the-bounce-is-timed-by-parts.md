# 0463 — A hitch frame is the watch's own span, and the bounce is timed by its parts
date: 2026-10-22
by: planner

## Decision
For 090 bug 12, carrying out the CPU half of 0462 under 0458 and 0461.

1. **The watch times its own frame.** The editor's hitch watch keeps `_begin`'s clock reading and
   hands app's frame rule the seconds from it to `_end`'s, not the tick's `elapsed`. Every stretch
   it lists then lies inside the frame it is listed with. What runs after `_end` and before the next
   `_begin` is in no frame, and the watch's header says so.
2. **A frame says how long it waited.** `voe_app_frame` gains `double waited`, the seconds
   `voe_app_frame_open` spent in 0215's pace loop, nought when focused or headless. The editor lists
   it as `pace: wait` within `pace`. What `pace` holds beyond it is the clock read and the poll.
3. **The bounce call times six parts.** With `frame->clock` set, `voe_3d_draw_bounce` adds to
   `voe_3d_shadow_times`: `bounce_fit` (the still casters' box and the grid fit), `bounce_stale`
   (the stale and removed spheres' walks), `bounce_begin` (render's bounce begins), `bounce_captures`
   (the capture passes, opened, drawn and closed), `bounce_suns` (the bounce sun maps with foliage)
   and `bounce_relight` (render's relights), each summed over the volumes. The rest of the call is in
   none. It also counts its calls in `uint32_t bounced`. The editor lists them as `bounce: fit`,
   `bounce: stale`, `bounce: begin`, `bounce: captures`, `bounce: suns`, `bounce: relight`, within
   `shadows: bounce`.
4. **The frame line ends with whether a light bounced**: `, light bounces` when any view's shadows
   call ran the bounce this frame, `, no light bounces` otherwise. The watch holds 24 stretches.
5. **The stall is fixed from the new lines** (0458 point 4): the sponsor files them as a bug of 090,
   and its fix is planned under 0386 from the part they name.

## Reasoning
- Bug 12's `pace` of 8002 ms inside a 4066 ms frame, and 13 s unnamed in another, are the tick's
  span (one clock read after the wait, before the poll, to the next) against the watch's (`_begin`
  to its marks). One span for both makes them add up by construction.
- `waited` tells the 0215 wait from a poll that stalls in one run; the poll is then `pace` less it.
- `shadows: bounce` at 538.7 of 543.1 ms names a call of six kinds of CPU work, none of whose headers
  says it can take a second; a fix of the guessed one is what 0458 was written against.
- Six flat fields over a nested struct: one struct to sum, as the editor already sums the five.

## Replaces
Nothing. Extends 0461 points 2 and 3.
