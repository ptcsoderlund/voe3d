# 01 — The slow heartbeat costs as much as drawing flat out

## Seen
"The editor's frame pacing does not reduce CPU when the window is unfocused."

Measured on KDE Plasma / Wayland, RTX 4070 Laptop, `build/debug`, one `voe_editor` process:

```
focused:    26-31% CPU, 757 voluntary wakeups/s
unfocused:  28% CPU,    368 voluntary wakeups/s
```

The wakeups halve while the CPU does not move at all. That is the signature of a busy-wait: the loop
stops blocking but keeps drawing every frame, so the unfocused editor costs a focused editor's worth
of machine.

What the human found in the code, so the planner does not have to find it twice. `wait_for_pace` in
`app/src/app.c` calls `voe_platform_window_wait` once and takes it on trust that it blocked for the
whole duration it asked for. `window_wayland.c` guards its `poll()` with `dispatched == 0 &&`, so it
returns straight away whenever anything was dispatched while draining the queue — card 02's
deliberate behaviour, not a mistake in itself. An unfocused window still gets Wayland events every
frame, so the wait never sleeps and the quarter-second deadline is never enforced. `app/src/pace.c`
is right: the pure function returns the number it should. Card 05's tests cover `pace.c` alone, which
is why the suite passes while the editor misbehaves.

Worth looking at in the same breath, unmeasured: `voe_platform_window_visible` returns `!suspended`,
and not every compositor sends the `suspended` xdg_toplevel state. On KDE the "not on screen at all,
stop drawing entirely" path may never fire, so steps 4 and 5 of the feature's test would fail for a
second, separate reason.

Card 05 sits in `done/` with this Done-when never verified.

## Expected
Unfocused, the editor costs near nothing: well under 5% of one core, as card 05 set out. Minimised,
on another virtual desktop, or fully covered, it costs nothing at all. Click back to it and the
picture is correct and up to date with no flash and no catching up.

Whatever the loop does to get there, it must be provable by the suite from now on — card 05's tests
could not fail on this, and the next change to the loop must not be able to bring it back quietly.

## How to reproduce
1. `cmake --build --preset debug` and start `voe_editor` with a scene open.
2. With the editor focused, run `ps -o pcpu=,rss= -C voe_editor` in a terminal. Note the figure.
3. Click the terminal so the editor loses focus. Wait a few seconds and run the same command.
4. The CPU figure is unchanged — around 28% — where it should be under 5%.
5. For the second half: minimise the editor, wait, and run it again. The CPU figure does not drop to
   zero either.
