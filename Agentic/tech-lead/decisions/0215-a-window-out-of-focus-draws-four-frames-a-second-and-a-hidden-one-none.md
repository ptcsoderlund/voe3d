# 0215 — A window out of focus draws four frames a second, and a hidden one none
date: 2026-09-22
by: planner

## Decision
Frame pacing is `app`'s, inside `voe_app_frame_open`, so every windowed program
gets it without asking. Focused, a frame is paced by the present (FIFO) and
nothing more. Not focused, `frame_open` waits until a quarter of a second has
passed since the previous frame opened, and returns early the moment the window
has any event, so a click back is drawn at once. Hidden, it draws nothing: it
waits on the window's events with no timeout until the window is shown again or
asked to close. A headless app is never paced.

`platform` answers the two questions and offers the wait: a window says whether
it is focused and whether it is visible, and can block until its connection has
an event or a timeout passes. On Wayland focus is the toplevel's `activated`
state and visible is the absence of `suspended` (xdg-shell 6, so `xdg_wm_base` is
bound at up to version 6); a compositor without it leaves the window visible. On
Windows focus is activation and visible is not minimised; a window on another
virtual desktop is not detected there.

`voe_dev` opens on FIFO like the editor. Its key that asks for MAILBOX stays: a
person pressing it is asking to measure the uncapped rate.

## Reasoning
Putting the pacing in `app` is one change for both programs instead of one per
loop, and the loop stays the program's (ADR-0135): `frame_open` already polls,
and waiting is the other half of polling. Four frames a second is "a few": the
picture is at most a quarter second stale when looked at, and the cost is a
rounding error. Waking on any event rather than sleeping out the quarter second
is what makes regaining focus instant. Skipping the draw but keeping the loop
spinning when hidden was rejected: it still burns a core polling. Detecting
occlusion by the absence of frame callbacks was rejected for now: the swapchain
owns the commits, and `suspended` is what the compositor sends for the cases the
feature names.

## Replaces
Amends ADR-0131 for `voe_dev`, which asked for MAILBOX at startup.
