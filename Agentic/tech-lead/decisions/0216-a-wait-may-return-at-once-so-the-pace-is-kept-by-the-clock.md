# 0216 — A wait may return at once, so the pace is kept by the clock
date: 2026-09-22
by: planner

## Decision
`voe_platform_window_wait`'s `seconds` is a ceiling and not a promise: the call
is allowed to return the moment anything at all arrives on the window's
connection, and a caller that needs a deadline loops and asks a clock.

`voe_app_frame_open` therefore keeps the pace as a loop, not as one wait. Each
turn asks the window whether it is focused, visible and closing, reads the
clock, and asks a pure step function for one of two answers: draw now, or wait
this long. A wait is followed by the usual `_poll`, and then the step is asked
again. The heartbeat ends when the clock says a quarter of a second has passed
since the previous frame opened — not when an event turns up.

What still ends the heartbeat early is a change the step can see: focus
regained, the window shown again, or a close request. Each is folded by the
`_poll` after the wait, so the next turn of the loop answers draw at once. A
remaining wait under a millisecond counts as draw, because a timeout truncated
to whole milliseconds would be no wait at all.

A compositor whose `xdg_wm_base` binds below version 6 cannot say `suspended`,
so its windows always read visible. That is a gap in what can be known, and the
Wayland window says so once, at open, rather than leaving it to be guessed at.

## Reasoning
An unfocused Wayland window still gets traffic on its connection every frame —
its own presentation and buffer releases among it. "The wait ends on any event"
therefore meant the wait never slept: bug 017/01 measured an unfocused editor at
the same 28% of a core as a focused one, with the wakeups merely halved, which
is a busy-wait wearing a heartbeat's clothes.

Teaching `platform` which events are worth waking for was rejected. It would
have to dispatch inside the wait, ahead of `voe_platform_input_begin_poll`,
which is what separates one frame's motion from the last one's; and it would put
the judgement of what a program cares about in the folder that knows least about
it. Looping on the clock leaves the wait dumb and keeps the pace where the pace
already lives.

The loop is a pure function of three booleans and two clock readings, so
`app/tests/pace.c` can drive an event storm — a wait that returns at once, every
time — and prove that no frame opens before the heartbeat has passed. Card 05's
tests could not fail on this; these can, and the bug says they must.

## Replaces
Amends 0215: the mechanism of "woken early by any event" becomes the loop above.
The three states and the quarter second stand unchanged.
