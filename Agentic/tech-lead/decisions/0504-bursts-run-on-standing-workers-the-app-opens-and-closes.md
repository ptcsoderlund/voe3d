# 0504 — Bursts run on standing workers the app opens and closes
date: 2026-10-30
by: planner

## Decision
For 110, carrying out 0497:
1. **`platform/burst.h` keeps every call it has** (`_cores`, `_burst_workers`, `_start`, `_done`,
   `_finish`), so no caller changes. It gains `voe_platform_workers_open()`, which starts
   max(2, cores) standing workers once, `voe_platform_workers_close()`, which ends and joins them,
   and `voe_platform_workers_standing()`, how many stand now. A start with none standing opens
   them first (tests, tools). The count is fixed from open to close.
2. **A burst is work handed to the standing workers.** At most min(workers, count) of them take its
   indices from its one atomic counter; bursts are served oldest first. Idle workers and a waiting
   finish sleep on condition variables: nothing spins or polls between bursts. `_burst_workers`
   is unchanged (cores − 1 under a splash, cores / 4 running), so each burst stays as wide as now.
   The caller runs no task; only when no worker stands do the tasks run on the caller, inside start.
3. **One level of nesting.** A burst's task may start and finish bursts (the splash wait's work
   does); a task of such a nested burst may not, asserted. max(2, cores) standing means a task
   waiting on its nested burst still leaves cores − 1 workers for it.
4. **`app` opens them and closes them**: `voe_app_new` and `voe_app_new_headless` open the workers
   on success, `voe_app_destroy` closes them. So the editor, the game (Play's and a shipped build)
   and dev start their workers when they open and end them when they close.
5. **The splash wait (`game/starting.h`) is a one-task, one-worker burst**, not a `thrd_t` of its
   own: Play's cook, Editor New and Open and scene loads run their wait on a standing worker.

## Reasoning
0497 point 1 says no thread is created between opening and closing, and 110's test 5 presses Play,
whose cook runs as a splash wait, and expects no thread to come and go; so the wait counts. Keeping
the burst calls means the model batch, a material's load and the compression queue need no change.
Alternatives: a finish that runs tasks on the waiting thread avoids the extra standing worker but
runs a task inside the outer work's report state (its first error, 0490, and subject stack, 0445);
opening only lazily at the first burst would make threads at a project's first import mid-run; a
standing thread of `game`'s own for the wait would be a second facility, which 0485 point 2 forbids.

## Replaces
Amends 0485 point 1 (a burst's work ends; its workers stay) and 0487 points 1 and 2 ("no thread
outlives its burst" and the Reasoning's rejected pool, both withdrawn by 0497). Amends 0370 points
1 and 2 and 0362's one worker: the wait's worker is a standing one and `game` names no `thrd_t`.
