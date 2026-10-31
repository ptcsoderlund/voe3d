# 01 — Bursts run on standing workers
folder: platform
after: none
decisions: 0168, 0497, 0504, 0485, 0487, 0490

## Change
The burst facility stops making and joining threads per burst; it hands each burst to a fixed set of
standing workers (0504 points 1–3). Every existing call keeps its signature and meaning to callers.

`platform/include/platform/burst.h`:
- Add `void voe_platform_workers_open(void)`: starts max(2, cores) workers once; a second open while
  they stand does nothing. Fewer made than asked (thread creation refused) keeps those made.
- Add `void voe_platform_workers_close(void)`: ends and joins every standing worker; asserts no burst
  is unfinished. Nothing standing: does nothing.
- Add `uint32_t voe_platform_workers_standing(void)`: workers standing now, 0 before open or after close.
- `voe_platform_burst_start`: opens the workers first when none stand; hands the burst to them, at
  most min(workers, count) taking its indices; asserts it is not called from a task of a burst that
  was itself started inside a task (one level of nesting, 0504 point 3).
- Rewrite the header's paragraphs: "bursts and not a pool" becomes standing workers started once and
  ended once (0497), sleeping between bursts on condition variables, never spinning or polling; why
  max(2, cores) (a task waiting on its nested burst leaves cores − 1); why the count of a burst
  still comes from `_burst_workers`; caller runs no task, only with no worker standing do tasks run
  on the caller inside start; a worker's thread is reused, so a task clears its first error (0490);
  the burst's memory still its own; open, close and the nesting rule.

`platform/src/burst.c`: the standing workers as file state behind one `mtx_t` with `cnd_t`s: a queue
of live bursts served oldest first; a worker takes the oldest burst that still has room for a worker
and indices left, runs its tasks from the burst's atomic counter, and sleeps when there is none.
`_done` stays a look that never waits; `_finish` sleeps on a condition variable until every task has
ended, then removes and frees the burst. A thread_local depth marks a task's thread for the nesting
assert. C11 threads only, one source for both systems; `burst_wayland.c` and `burst_win32.c` unchanged.

`platform/tests/burst.c`: keep the existing checks; add one that opens, runs 50 bursts whose tasks
record `thrd_current()`, and checks the standing count is unchanged throughout and every recorded
thread is one of at most that many distinct threads; one where a task of a one-worker burst starts
and finishes a `voe_platform_burst_workers(true)` burst and every inner task ran; and close bringing
the standing count to 0, a later start opening them again.

Index entries to update: `platform/platform.md`'s `burst.h` line (standing workers, open, close; why
standing and not per burst); `platform/src/src.md`'s `burst.c` line; `platform/tests/tests.md`'s
`burst.c` line.

## Meaning
- Holds: one engine facility runs all parallel work; nothing spins or polls between bursts; a burst
  never takes every core (0485 points 1–3, 0386).
- Holds: a task clears its thread's first error and copies it into its record; the caller keeps the
  texts in task order (0490). Workers now persist, so the clear matters more, not less.
- Holds: the users are the model batch, a material's load and the editor's compression queue; none
  of them changes on this card.

## Done when
The test `platform/burst` passes with the new checks above, and `grep -c thrd_create
platform/src/burst.c` counts only the one call inside `voe_platform_workers_open`.
