# 03 — The splash wait runs on a standing worker
folder: game
after: 01, 02
decisions: 0168, 0497, 0504, 0370, 0362

## Change
The splash wait stops making its own thread; its work runs as a one-task, one-worker burst on the
standing workers (0504 point 5), so Play's cook, Editor New and Open and scene loads create no thread.

`game/src/starting.c`, `voe_game_starting_wait`: replace the `thrd_t`, `thrd_create` and
`thrd_join` with `voe_platform_burst_start(1, 1, <task>, &worker)` (`platform/burst.h`), the task
running the work into `worker.answer`; the frame loop runs while `!voe_platform_burst_done`; a
false frame sets the progress's stop, then `voe_platform_burst_finish`, then answers false; the
normal end finishes the burst before reading the answer. The "no thread to be had" branch goes:
a start never fails, and with no worker standing it runs the work inside start. Drop the
`<threads.h>` include and `ended` atomic if nothing else uses them.

`game/include/game/starting.h`: "THE WAIT'S TWO THREADS" says the work runs on one standing worker
as a one-task burst, not a thread of its own (0504), the rest of that paragraph unchanged; the
wait's declaration comment says "one standing worker".

Index entries to update: `game/src/src.md`'s `starting.c` line ("the wait's worker thread" becomes
the wait's one-task burst).

## Meaning
- Holds: Play's and Ship's cook runs in the editor's process as one of the splash-wait works in
  `editor/src/loading.h`, and inside it the model batch starts a burst per phase; that is the
  nesting 0504 point 3 allows.
- Holds: outside tests only `platform/src/burst.c` and `game/src/starting.c` create threads; after
  this card only the first does.

## Done when
`game/tests/starting.c` gains a check that `voe_platform_workers_standing()` is the same before and
after the wait around the shaders step; the test `game/starting` passes, and
`grep -rn thrd_create game/src` prints nothing.

The human's, not the coder's (feature.md `## How to test`):
1. Under CLion's debugger on Windows, open the hill maximized; after the splash, a minute's flying
   shows no "thread created" or "thread exited" lines.
2. Still under it: drop a new model into Assets, change a model file, import a texture; each shows
   up as before and no threads come and go.
3. Flying through the forest under the debugger does not freeze.
4. Without the debugger, the hill opens from cold about as fast as before, or faster.
5. Play, then stop: no threads come and go; closing the editor is at once and clean.
6. Ship the hill and run the build: it opens and runs as before.
