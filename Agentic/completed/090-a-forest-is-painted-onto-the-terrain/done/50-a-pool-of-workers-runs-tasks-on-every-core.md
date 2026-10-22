# 50 — A pool of workers runs tasks on every core
folder: base
after: 49
decisions: 0168, 0370, 0447, 0448

## Change
0448 point 1: the one pool the loaders run on. Standard C threads, as 0370 point 1 argues.

- `base/include/base/workers.h` — new. `typedef struct voe_base_workers voe_base_workers;`,
  `typedef void voe_base_workers_task(void *context, uint32_t index);` and:
  - `voe_base_workers *voe_base_workers_new(uint32_t threads)` — starts `threads` threads
    (0 allowed: a pool that runs everything on its callers).
  - `void voe_base_workers_destroy(voe_base_workers *workers)` — stops and joins them; no run
    may be in progress. NULL does nothing.
  - `uint32_t voe_base_workers_threads(const voe_base_workers *workers)` — its threads plus one
    for the caller; 1 for NULL.
  - `void voe_base_workers_run(voe_base_workers *workers, uint32_t count,
    voe_base_workers_task *task, void *context)` — runs `task(context, i)` once for each `i`
    below `count` and returns when all have returned. The caller runs tasks too; NULL or no
    threads runs them in order on the caller.
  Header: fork and join, no job system (0362); runs may nest (a task may run) and come from
  several threads at once, a waiting thread running any queued task meanwhile, so no wait
  deadlocks; a task gets an index and no thread number, so it makes its own arena (an arena is
  one thread's, base/arena.h); each task runs under the subjects its run's caller had pushed,
  the thread's own put back after; the kept first error of the lowest-numbered task that kept
  one becomes the caller's when the caller kept none (base/report.h, 0445); a task must not
  push past `VOE_BASE_REPORT_SUBJECTS` with its caller's.
- `base/src/workers.c` — new: the threads, one queue of runs under an `mtx_t` and `cnd_t`,
  counts in `<stdatomic.h>`; each task between card 49's `voe_base_report_state_save` and
  `_load` (the run's subjects, no kept error), what it kept handed to its run.
- `base/include/base/base.md`, `base/base.md` — entries for `workers.h`. `base/src/src.md` — an
  entry for `workers.c`.
- `base/tests/workers.c` — new: NULL and a pool of 0 run 100 tasks in order on the caller; a
  pool of 3 runs 10 000 tasks each exactly once (a flag per index, a sum); 50 tasks each running
  20 more nest on a pool of 1 and of 3 and all finish; two threads of the test running on one
  pool at once both finish; with "Assets/a.glb" pushed, tasks 7 and 3 each reporting an error
  leave the caller's kept error beginning "Assets/a.glb: " with task 3's message, and a caller
  that kept one before keeps its own; destroy after runs returns.
- `base/tests/tests.md` — an entry for `workers.c`.

## Done when
`ctest --test-dir build/debug -R '^base/(workers|report)$'` passes.
