// The burst of platform/burst.h on C11 threads, one source for both systems:
// read that header first. Only the core count is per system, in
// burst_wayland.c and burst_win32.c.
//
// Workers take indices from `next`, one atomic fetch-and-add each, and count
// each task they end into `ended`; done is `ended == count`, read with acquire
// so a caller that sees it also sees every task's writes. `next` is 64 bits so
// the one overshoot per worker past `count` cannot wrap.
//
// Constraints: the workers live in a flexible array sized at start, one
// allocation per burst. A thread that will not start ends the starting there;
// the workers already made take every task, and with none the caller runs them.
#include <platform/burst.h>

#include <base/assert.h>

#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>

struct voe_platform_burst {
	voe_platform_burst_task *task;
	void *context;
	uint32_t count;
	uint32_t started;
	_Atomic uint64_t next;
	_Atomic uint32_t ended;
	thrd_t workers[];
};

uint32_t voe_platform_burst_workers(bool splash)
{
	uint32_t cores = voe_platform_cores();
	uint32_t workers = splash ? cores - 1 : cores / 4;

	return workers < 1 ? 1 : workers;
}

// Takes the next index until none is left; every take is bounded by count.
static void burst_run(voe_platform_burst *burst)
{
	for (;;) {
		uint64_t index = atomic_fetch_add_explicit(&burst->next, 1,
							   memory_order_relaxed);

		if (index >= burst->count)
			return;
		burst->task(burst->context, (uint32_t)index);
		atomic_fetch_add_explicit(&burst->ended, 1, memory_order_release);
	}
}

static int burst_worker(void *argument)
{
	burst_run(argument);
	return 0;
}

voe_platform_burst *voe_platform_burst_start(uint32_t workers, uint32_t count,
					     voe_platform_burst_task *task,
					     void *context)
{
	uint32_t wanted = workers < count ? workers : count;
	voe_platform_burst *burst;

	VOE_BASE_ASSERT(workers >= 1, "a burst needs at least one worker");
	VOE_BASE_ASSERT(task != NULL, "a burst needs a task");
	burst = calloc(1, sizeof(*burst) + wanted * sizeof(burst->workers[0]));
	VOE_BASE_ASSERT(burst != NULL, "no room for a burst");
	burst->task = task;
	burst->context = context;
	burst->count = count;
	atomic_init(&burst->next, 0);
	atomic_init(&burst->ended, 0);

	while (burst->started < wanted &&
	       thrd_create(&burst->workers[burst->started], burst_worker, burst) ==
		       thrd_success)
		burst->started++;
	if (burst->started == 0)
		burst_run(burst);
	return burst;
}

bool voe_platform_burst_done(const voe_platform_burst *burst)
{
	VOE_BASE_ASSERT(burst != NULL, "done asked of no burst");
	return atomic_load_explicit(&burst->ended, memory_order_acquire) ==
	       burst->count;
}

void voe_platform_burst_finish(voe_platform_burst *burst)
{
	VOE_BASE_ASSERT(burst != NULL, "finish asked of no burst");
	for (uint32_t i = 0; i < burst->started; i++)
		(void)thrd_join(burst->workers[i], NULL);
	VOE_BASE_ASSERT(voe_platform_burst_done(burst),
			"every worker joined, so every task ended");
	free(burst);
}
