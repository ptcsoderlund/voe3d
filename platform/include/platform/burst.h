// A burst: one piece of work split into `count` tasks and run on a few worker
// threads that are started for it and joined after it (ADR-0485, ADR-0487).
//
//     voe_platform_burst *burst = voe_platform_burst_start(
//             voe_platform_burst_workers(splash), count, decode_one, pictures);
//     ... the caller's own loop, polling voe_platform_burst_done(burst) ...
//     voe_platform_burst_finish(burst);
//
// BURSTS AND NOT A POOL (0485 point 1). No thread outlives its burst, so
// nothing parks, spins or polls between bursts and an idle program costs
// nothing. A pool of parked threads is the permanent system 0485 rejected.
//
// A BURST NEVER TAKES EVERY CORE. voe_platform_burst_workers gives cores - 1
// under a splash and a quarter of them while the program runs, so the main
// thread and the GPU submission keep theirs (0386). The core count is asked of
// the system once and kept; it is at least 1, and so is every worker count.
//
// THE CALLER RUNS NO TASK, so its own thread keeps its core and can keep a
// window answering. Only when no worker will start (no thread could be made)
// do the tasks run on the caller, inside start, which therefore never fails.
// A start never makes more workers than there are tasks.
//
// TASKS RUN IN ANY ORDER, ON ANY WORKER, AT ONCE. Workers take indices in turn
// from one atomic counter, so a task touches only the data of its own index
// and reads what nothing writes while the burst runs. Its report subject
// (base/report.h) is its own thread's: a task names its own subjects.
//
// THE BURST'S MEMORY IS ITS OWN, allocated at start and freed at finish: the
// exception rule 11 makes for data that outlives a call, since a burst runs
// past the start that made it. A task is a function pointer because a burst
// runs any work; the context is the caller's and the burst never reads it.
//
// _done is a look that never waits; _finish waits for every task, joins every
// worker and frees the burst, and every burst started is finished exactly once.
#pragma once

#include <stdint.h>

// Logical processors on this machine, asked once and kept; at least 1.
uint32_t voe_platform_cores(void);

// Workers for a burst: cores - 1 under a splash, else cores / 4; at least 1.
uint32_t voe_platform_burst_workers(bool splash);

// One task: `context` as given to start, and this task's index, below count.
typedef void voe_platform_burst_task(void *context, uint32_t index);

typedef struct voe_platform_burst voe_platform_burst;

// Starts at most min(workers, count) workers over tasks 0 .. count - 1.
// Never NULL; `workers` is at least 1 and `task` is not NULL.
voe_platform_burst *voe_platform_burst_start(uint32_t workers, uint32_t count,
					     voe_platform_burst_task *task,
					     void *context);

// Whether every task has ended. Never waits.
bool voe_platform_burst_done(const voe_platform_burst *burst);

// Waits for every task, joins every worker and frees the burst.
void voe_platform_burst_finish(voe_platform_burst *burst);
