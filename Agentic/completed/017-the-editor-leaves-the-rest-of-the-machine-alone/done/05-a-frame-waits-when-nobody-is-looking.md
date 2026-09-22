# 05 — A frame waits when nobody is looking
folder: app
decisions: 0168, 0215

## Change
Files: new `app/src/pace.h` and `app/src/pace.c`, `app/src/app.c`, `app/include/app/app.h`, new
`app/tests/pace.c`, `app/tests/tests.md`, `app/src/src.md`.

`pace.h` — internal. The decision alone, pure, so a test needs no window:

```c
#define VOE_APP_HEARTBEAT_SECONDS 0.25
double voe_app_pace_wait(bool focused, bool visible, double now, double last_open);
```

Returns how long `frame_open` should wait: 0 focused; negative (no timeout) when not visible;
otherwise what is left of the heartbeat since `last_open`, never below 0. Its header says why the
wait ends early on any event, why hidden is no timeout and not a long heartbeat, and cites 0215.

`app.c`: the app keeps the clock reading at which the last frame opened. `voe_app_frame_open`, on a
window, before it reads the clock, asks `voe_platform_window_focused` and `_visible` (the last poll's
answers) and the pace. Unfocused and visible: one `voe_platform_window_wait` for what the pace says
(an event ends it early) and a `voe_platform_window_poll`. Hidden: wait with no timeout and poll,
repeated until visible or closing. Then reads the clock, ticks and polls as today. Headless:
unchanged, never paced.

`app.h`: the header's example loop is unchanged; `voe_app_frame_open`'s comment loses "it does not
wait, so a loop with nothing else in it spins a core" and says what it now waits for in each of the
three states, that closing always ends a wait, and that the step after a long wait is clamped by
`longest_step` as any stall is. `app/app.md`'s `app.h` entry mentions the pacing.

`app/tests/pace.c` — new, includes `../src/pace.h`. Claims: focused is 0 whatever the times;
hidden is negative, focused or not; unfocused just after a frame waits the rest of the heartbeat;
unfocused a whole heartbeat or more after waits 0. `tests.md` gains it.

## Done when
`checks.sh --folder app` exits 0 including test `app/pace`; after `cmake --build --preset debug
--target voe_editor`, with `voe_editor` running and another window focused,
`ps -o pcpu= -C voe_editor` reads under 5.
