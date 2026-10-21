// The pool behind include/base/workers.h: its threads, one queue of runs under
// one mtx_t, and one cnd_t that wakes both idle threads and waiting callers.
//
// A run lives on its caller's stack. It is queued while it has an index left
// to claim; an index is claimed under the lock, and the count of finished
// tasks is atomic, so finishing a task takes the lock only for the last one,
// to wake the caller. Nothing touches a run after its task is counted: the
// caller may have returned and its stack be another run's.
//
// Each task runs between report_state.h's save and load: the thread's own
// state saved, the run's caller's subjects loaded with no error kept, and
// after the task what it kept handed to the run, the lowest index winning,
// before the thread's own state is loaded back.
//
// Constraints: a waiting thread runs whatever task is queued, so a long task
// of another run can hold up a caller whose own tasks are done, and a helped
// task that runs again deepens the waiting thread's stack (two report states,
// about 3 KB, a level). Runs are taken newest first, which keeps nested runs
// short; a run that must be served first would need a priority, which
// ADR-0362 leaves out.
#include "report_state.h"

#include <base/assert.h>
#include <base/workers.h>

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

struct run {
	voe_base_workers_task *task;
	void *context;
	uint32_t count;
	uint32_t next;	      // the next index to claim, under the pool's lock
	atomic_uint finished; // tasks that have returned
	struct run *queued_next;
	// The caller's subjects, no error kept: what each task runs under.
	struct voe_base_report_state caller;
	// The kept error of the lowest task to keep one, under the pool's lock.
	uint32_t error_index;
	char error[VOE_BASE_REPORT_LINE_CAPACITY];
};

struct voe_base_workers {
	mtx_t lock;
	cnd_t wake;
	struct run *queue;
	bool stopping;
	uint32_t thread_count;
	thrd_t threads[];
};

// Runs one task under the run's report state, hands on what it kept, and
// counts it. `workers` is NULL when the caller runs alone, and nothing locks.
static void run_task(struct run *run, uint32_t index, voe_base_workers *workers)
{
	struct voe_base_report_state own;

	voe_base_report_state_save(&own);
	voe_base_report_state_load(&run->caller);
	run->task(run->context, index);

	const char *kept = voe_base_report_error_first();
	if (kept != NULL) {
		if (workers != NULL)
			mtx_lock(&workers->lock);
		if (index < run->error_index) {
			run->error_index = index;
			snprintf(run->error, sizeof(run->error), "%s", kept);
		}
		if (workers != NULL)
			mtx_unlock(&workers->lock);
	}
	voe_base_report_state_load(&own);

	// Read before counting: once the last task is counted the run may be gone.
	const uint32_t count = run->count;
	uint32_t done = atomic_fetch_add(&run->finished, 1) + 1;
	VOE_BASE_ASSERT(done <= count, "a task counted twice");
	if (done == count && workers != NULL) {
		mtx_lock(&workers->lock);
		cnd_broadcast(&workers->wake);
		mtx_unlock(&workers->lock);
	}
}

// Under the lock: takes the next index of the newest queued run, dequeuing
// the run once its last index is taken. False when nothing is queued.
static bool claim_task(voe_base_workers *workers, struct run **run,
		       uint32_t *index)
{
	struct run *head = workers->queue;

	if (head == NULL)
		return false;
	VOE_BASE_ASSERT(head->next < head->count, "a queued run has an index left");
	*run = head;
	*index = head->next++;
	if (head->next == head->count)
		workers->queue = head->queued_next;
	return true;
}

static int worker_main(void *argument)
{
	voe_base_workers *workers = argument;
	struct run *run;
	uint32_t index;

	mtx_lock(&workers->lock);
	while (!workers->stopping) {
		if (claim_task(workers, &run, &index)) {
			mtx_unlock(&workers->lock);
			run_task(run, index, workers);
			mtx_lock(&workers->lock);
		} else {
			cnd_wait(&workers->wake, &workers->lock);
		}
	}
	mtx_unlock(&workers->lock);
	return 0;
}

voe_base_workers *voe_base_workers_new(uint32_t threads)
{
	voe_base_workers *workers =
		malloc(sizeof(*workers) + (size_t)threads * sizeof(thrd_t));
	VOE_BASE_ASSERT(workers != NULL, "out of memory allocating a pool");

	VOE_BASE_ASSERT(mtx_init(&workers->lock, mtx_plain) == thrd_success,
			"the pool's lock did not initialise");
	VOE_BASE_ASSERT(cnd_init(&workers->wake) == thrd_success,
			"the pool's condition did not initialise");
	workers->queue = NULL;
	workers->stopping = false;
	workers->thread_count = 0;

	for (uint32_t i = 0; i < threads; i++) {
		if (thrd_create(&workers->threads[i], worker_main, workers) !=
		    thrd_success) {
			VOE_BASE_WARNING("base", "started %u of %u worker threads",
					 (unsigned)i, (unsigned)threads);
			break;
		}
		workers->thread_count = i + 1;
	}
	return workers;
}

void voe_base_workers_destroy(voe_base_workers *workers)
{
	if (workers == NULL)
		return;

	mtx_lock(&workers->lock);
	VOE_BASE_ASSERT(workers->queue == NULL, "a pool destroyed during a run");
	workers->stopping = true;
	cnd_broadcast(&workers->wake);
	mtx_unlock(&workers->lock);

	for (uint32_t i = 0; i < workers->thread_count; i++)
		thrd_join(workers->threads[i], NULL);
	cnd_destroy(&workers->wake);
	mtx_destroy(&workers->lock);
	free(workers);
}

uint32_t voe_base_workers_threads(const voe_base_workers *workers)
{
	return workers == NULL ? 1 : workers->thread_count + 1;
}

// Queues the run and, until its last task has returned, runs whatever task is
// queued, sleeping when none is.
static void run_on_pool(voe_base_workers *workers, struct run *run)
{
	struct run *claimed;
	uint32_t index;

	mtx_lock(&workers->lock);
	run->queued_next = workers->queue;
	workers->queue = run;
	cnd_broadcast(&workers->wake);

	while (atomic_load(&run->finished) < run->count) {
		if (claim_task(workers, &claimed, &index)) {
			mtx_unlock(&workers->lock);
			run_task(claimed, index, workers);
			mtx_lock(&workers->lock);
		} else {
			cnd_wait(&workers->wake, &workers->lock);
		}
	}
	mtx_unlock(&workers->lock);
}

void voe_base_workers_run(voe_base_workers *workers, uint32_t count,
			  voe_base_workers_task *task, void *context)
{
	VOE_BASE_ASSERT(task != NULL, "a run has a task");
	if (count == 0)
		return;

	struct run run = {
		.task = task,
		.context = context,
		.count = count,
		.error_index = UINT32_MAX,
	};
	voe_base_report_state_save(&run.caller);
	run.caller.first_error_kept = false;

	if (workers == NULL || workers->thread_count == 0) {
		for (uint32_t i = 0; i < count; i++)
			run_task(&run, i, NULL);
	} else {
		run_on_pool(workers, &run);
	}

	// Every task has returned, so the run's state is the caller's alone.
	if (run.error_index != UINT32_MAX &&
	    voe_base_report_error_first() == NULL) {
		voe_base_report_state_save(&run.caller);
		snprintf(run.caller.first_error, sizeof(run.caller.first_error),
			 "%s", run.error);
		run.caller.first_error_kept = true;
		voe_base_report_state_load(&run.caller);
	}
}
