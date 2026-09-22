# 11 — The heartbeat is kept by the clock, and a storm cannot beat it
folder: app
decisions: 0168, 0215, 0216
read: feature.md

## Change
The pace waits once and trusts the wait; an unfocused window gets connection
traffic every frame, so it never sleeps and draws flat out (bug 017/01). The
pace becomes a loop over a pure step, kept by the clock.

`app/src/pace.h` — replace `voe_app_pace_wait` with the step, keeping
`VOE_APP_HEARTBEAT_SECONDS` and adding a floor of one millisecond:

```c
typedef enum { VOE_APP_PACE_DRAW, VOE_APP_PACE_WAIT } voe_app_pace_kind;
typedef struct { voe_app_pace_kind kind; double seconds; } voe_app_pace_step;
voe_app_pace_step voe_app_pace_next(bool focused, bool visible, bool closing,
				    double now, double last_open);
```

Closing draws, from every state and before anything else is asked. Not visible
waits with a negative `seconds` — no timeout. Focused draws. Otherwise it waits
the rest of the heartbeat since `last_open`, and draws once that rest is below
the floor. The header keeps the asserts' spirit and rewrites its reasoning: the
loop this is one step of; that a wait may return at any moment, so the clock and
not the event decides when the heartbeat is over (ADR-0216); that focus
regained, being shown and a close request all end it early because the `_poll`
after each wait folds them and the next step sees them; why hidden is no timeout
rather than a long one; why closing wins over hidden; and why the floor exists —
a timeout truncated to whole milliseconds is no wait at all and would spin.

`app/src/pace.c` — the cases in that order.

`app/src/app.c` — `wait_for_pace` becomes the loop and loses both of its present
branches: read `voe_platform_window_focused`, `_visible`,
`_should_close` and `voe_platform_clock_now`, ask `voe_app_pace_next`; on draw
return; on wait call `voe_platform_window_wait` with its seconds, then
`voe_platform_window_poll`, then round again. Its comment says the loop, not the
wait, is what enforces the heartbeat, and what ends it early.

`app/include/app/app.h` — the "IT WAITS WHEN NOBODY IS LOOKING" paragraph keeps
its three states, the clamp and the headless line, and replaces "woken early by
any event" with the loop: the wait is repeated until the clock says the quarter
second has passed, and what ends it early is regaining focus, being shown or a
close request (ADR-0216).

`app/tests/pace.c` — the existing claims against the new call, plus the two the
bug asks for. An event storm: drive the step in the loop app.c runs, with a fake
clock that advances one millisecond per wait however long the step asked for,
unfocused and visible throughout — no draw before a quarter second of clock has
passed, and the count of turns is finite. The same storm hidden — no draw at
all until the answers say visible, and none either until closing does. Closing
draws at once from focused, unfocused and hidden.

`app/tests/tests.md` — `pace.c`'s entry names the storm. `app/src/src.md` —
`pace.h` and `pace.c` follow the step.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder app` exits 0 including
test `app/pace`, and `bash ~/.claude/skills/checks/scripts/checks.sh --all`
exits 0.

Then the human, the feature's own test, on `build/debug`: steps 2, 3, 4 and 5
against `voe_editor` — focused well under a core, unfocused, on another desktop
and minimised near nothing, and the picture correct and up to date on the way
back — and step 9, the same for `voe_dev`.
