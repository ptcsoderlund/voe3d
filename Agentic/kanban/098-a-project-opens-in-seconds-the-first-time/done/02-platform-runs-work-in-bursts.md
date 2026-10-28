# 02 — platform runs work in bursts
folder: platform
after: none
decisions: 0168, 0485, 0487

## Change
0487 points 1 and 2. The folder gains a module; no other folder uses it yet.

- `platform/include/platform/burst.h` — new:
  - `uint32_t voe_platform_cores(void)` — logical processors, asked of the system once and kept,
    at least 1.
  - `uint32_t voe_platform_burst_workers(bool splash)` — cores − 1 under a splash, else
    cores / 4, at least 1.
  - `typedef void voe_platform_burst_task(void *context, uint32_t index);`
  - `typedef struct voe_platform_burst voe_platform_burst;`
  - `voe_platform_burst *voe_platform_burst_start(uint32_t workers, uint32_t count,
    voe_platform_burst_task *task, void *context)` — at most `count` workers, never NULL.
  - `bool voe_platform_burst_done(const voe_platform_burst *burst)` — every task ended, no wait.
  - `void voe_platform_burst_finish(voe_platform_burst *burst)` — waits, joins, frees.
  - Header points: why bursts and not a pool (0485 point 1); a burst never takes every core; the
    caller runs no task except when no worker starts; tasks run in any order on any worker, so a
    task touches only its own index's data, and its report subject is its own thread's
    (base/report.h); the burst's memory is its own (rule 11's exception); a function pointer
    here because a burst runs any work.
- `platform/src/burst.c` — new: start, done and finish on C11 `threads.h` with an atomic next
  index and an atomic count of ended tasks; `platform/src/burst_wayland.c` and
  `platform/src/burst_win32.c` — new: the core count (`sysconf` online processors;
  `GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)`).
- `platform/platform.md` — the opening list of what platform covers, and the `burst.h` entry;
  `platform/src/src.md` — the three files.

## Done when
`platform/tests/burst.c` (new, entry in `platform/tests/tests.md`) passes:
- `voe_platform_cores()` is at least 1 and the same on a second call; both worker counts are at
  least 1 and below the core count when it is above 1;
- 10 000 tasks on four workers each run once (a counter per index), `_done` turns true and stays;
- a burst of 0 tasks is done at once; one of 3 tasks with 8 workers runs each once.
