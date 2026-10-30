// A burst: one piece of work split into `count` tasks and run on the standing
// workers, a fixed set of threads started once and ended once (ADR-0485,
// ADR-0497, ADR-0504).
//
//     voe_platform_workers_open();          // when the program opens
//     voe_platform_burst *burst = voe_platform_burst_start(
//             voe_platform_burst_workers(splash), count, decode_one, pictures);
//     ... the caller's own loop, polling voe_platform_burst_done(burst) ...
//     voe_platform_burst_finish(burst);
//     voe_platform_workers_close();         // when the program closes
//
// STANDING WORKERS, STARTED ONCE AND ENDED ONCE (0497). No thread is made or
// ended between open and close, so a debugger that pauses on each one is not
// paused mid-run. Between bursts the workers sleep on condition variables:
// nothing spins or polls, and an idle program still costs nothing. A start
// with none standing opens them first, so a test or a tool needs no open; a
// second open while they stand does nothing; close asserts every burst is
// finished. Fewer made than asked (the system refused a thread) keeps those.
//
// MAX(2, CORES) STAND. A task may start and finish a burst of its own and
// waits on it while it runs; with one more worker than cores - 1, that nested
// burst still has the cores - 1 a splash burst would get. A task of such a
// nested burst starts no burst: one level of nesting, asserted.
//
// A BURST NEVER TAKES EVERY CORE. Its width still comes from
// voe_platform_burst_workers: cores - 1 under a splash and a quarter of them
// while the program runs, so the main thread and the GPU submission keep
// theirs (0386). At most that many workers take a burst's indices, whatever
// stands; bursts are served oldest first. The core count is asked of the
// system once and kept; it is at least 1, and so is every worker count.
//
// THE CALLER RUNS NO TASK, so its own thread keeps its core and can keep a
// window answering. Only when no worker stands (no thread could be made) do
// the tasks run on the caller, inside start, which therefore never fails.
//
// TASKS RUN IN ANY ORDER, ON ANY WORKER, AT ONCE. Workers take indices in turn
// from one atomic counter, so a task touches only the data of its own index
// and reads what nothing writes while the burst runs. Its report subject
// (base/report.h) is its own thread's, and that thread is reused from burst to
// burst: a task names its own subjects and clears its first error (0490).
//
// THE BURST'S MEMORY IS ITS OWN, allocated at start and freed at finish: the
// exception rule 11 makes for data that outlives a call, since a burst runs
// past the start that made it. A task is a function pointer because a burst
// runs any work; the context is the caller's and the burst never reads it.
//
// _done is a look that never waits; _finish sleeps until every task has ended
// and frees the burst, and every burst started is finished exactly once.
#pragma once

#include <stdint.h>

// Logical processors on this machine, asked once and kept; at least 1.
uint32_t voe_platform_cores(void);

// Workers for a burst: cores - 1 under a splash, else cores / 4; at least 1.
uint32_t voe_platform_burst_workers(bool splash);

// Starts max(2, cores) standing workers; nothing when they already stand.
void voe_platform_workers_open(void);

// Ends and joins every standing worker; no burst may be unfinished. Not from a task.
void voe_platform_workers_close(void);

// Workers standing now: 0 before open and after close.
uint32_t voe_platform_workers_standing(void);

// One task: `context` as given to start, and this task's index, below count.
typedef void voe_platform_burst_task(void *context, uint32_t index);

typedef struct voe_platform_burst voe_platform_burst;

// Hands tasks 0 .. count - 1 to at most min(workers, count) standing workers,
// opening them first when none stand. Never NULL; `workers` is at least 1 and
// `task` is not NULL; not called from a task of a nested burst.
voe_platform_burst *voe_platform_burst_start(uint32_t workers, uint32_t count,
					     voe_platform_burst_task *task,
					     void *context);

// Whether every task has ended. Never waits.
bool voe_platform_burst_done(const voe_platform_burst *burst);

// Sleeps until every task has ended, then frees the burst.
void voe_platform_burst_finish(voe_platform_burst *burst);
