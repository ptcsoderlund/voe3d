// The one pool of worker threads loading runs on (ADR-0448 point 1): a call
// runs one task for each index below a count and returns when all have
// returned. Fork and join, not a job system (ADR-0362): no priorities, no
// futures, no task that outlives its run. Standard C threads (ADR-0370).
//
//     voe_base_workers *workers = voe_base_workers_new(cores - 1);
//     voe_base_workers_run(workers, count, task, context);
//     voe_base_workers_destroy(workers);
//
// The caller runs tasks too, so a pool of n threads uses n + 1, and a pool of
// none, or NULL, runs the tasks in order on the caller.
//
// RUNS MAY NEST AND MAY COME FROM SEVERAL THREADS AT ONCE. A task may itself
// run, and two threads may run on one pool together. A thread waiting for its
// run runs any queued task meanwhile, of its own run or another's, so no wait
// deadlocks for want of a thread.
//
// A TASK GETS AN INDEX, NEVER A THREAD NUMBER. So it makes its own arena: an
// arena is one thread's (base/arena.h), and an arena per pool thread is unsafe
// once a waiting thread runs another run's task in the middle of its own.
//
// A TASK REPORTS AS ITS CALLER WOULD (base/report.h, ADR-0445). Each task runs
// under the subjects its run's caller had pushed and with no error kept; the
// thread's own subjects and kept error are put back after. The kept first
// error of the lowest-numbered task that kept one becomes the caller's when
// the caller kept none. A task may push subjects of its own, but not past
// VOE_BASE_REPORT_SUBJECTS counted with its caller's.
#pragma once

#include <stdint.h>

typedef struct voe_base_workers voe_base_workers;

// One task of a run: `index` is below the run's count, each index once.
typedef void voe_base_workers_task(void *context, uint32_t index);

// Starts `threads` threads; 0 is a pool that runs everything on its callers.
// A thread the system refuses is a warning, and the pool runs with fewer.
voe_base_workers *voe_base_workers_new(uint32_t threads);

// Stops and joins the threads. No run may be in progress. NULL does nothing.
void voe_base_workers_destroy(voe_base_workers *workers);

// Its threads plus one for the caller; 1 for NULL.
uint32_t voe_base_workers_threads(const voe_base_workers *workers);

// Runs task(context, i) once for each i below `count` and returns when all
// have returned. NULL or a pool of no threads runs them in order on the caller.
void voe_base_workers_run(voe_base_workers *workers, uint32_t count,
			  voe_base_workers_task *task, void *context);
