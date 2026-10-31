// The standing workers and the bursts of platform/burst.h on C11 threads, one
// source for both systems: read that header first. Only the core count is per
// system, in burst_wayland.c and burst_win32.c.
//
// The workers are `crew`, file state behind one mutex: the threads, how many
// stand, a stop flag and the queue of live bursts, a list linked through
// `later`, oldest first. A burst joins the queue at start and leaves it at
// finish. A worker takes the oldest burst that still has `room` for a worker
// and indices left, runs tasks from it until none is left, and sleeps on
// `work` when no burst has both. A finish sleeps on `left` until every task
// has ended and no worker is on its burst any more, so a burst is never freed
// under a worker still reaching for its next index.
//
// Workers take indices from `next`, one atomic fetch-and-add each, and count
// each task they end into `ended`; done is `ended == count`, read with acquire
// so a caller that sees it also sees every task's writes. `next` is 64 bits so
// the one overshoot per worker past `count` cannot wrap.
//
// `task_level` is the thread's own: the level of the burst whose task it runs
// now, 0 when none. A burst started there is one level deeper, and a start at
// level 2 is the nesting the header forbids.
//
// Constraints: the queue is walked under the lock, one step per live burst, a
// handful at most; a long-lived queue would want a tail pointer. Close is not
// called while another thread starts a burst. The thread array is allocated at
// open and freed at close.
#include <platform/burst.h>

#include <base/assert.h>

#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>

struct voe_platform_burst {
	voe_platform_burst_task *task;
	void *context;
	uint32_t count;
	uint32_t level;
	uint32_t room;
	uint32_t working;
	voe_platform_burst *later;
	_Atomic uint64_t next;
	_Atomic uint32_t ended;
};

static struct {
	mtx_t lock;
	cnd_t work;
	cnd_t left;
	thrd_t *threads;
	uint32_t standing;
	bool stopping;
	voe_platform_burst *oldest;
} crew;

static once_flag crew_once = ONCE_FLAG_INIT;
static thread_local uint32_t task_level;

static void crew_init(void)
{
	VOE_BASE_ASSERT(mtx_init(&crew.lock, mtx_plain) == thrd_success,
			"the workers' lock could not be made");
	VOE_BASE_ASSERT(cnd_init(&crew.work) == thrd_success,
			"the workers' wake could not be made");
	VOE_BASE_ASSERT(cnd_init(&crew.left) == thrd_success,
			"the finishers' wake could not be made");
}

static void crew_lock(void)
{
	call_once(&crew_once, crew_init);
	VOE_BASE_ASSERT(mtx_lock(&crew.lock) == thrd_success,
			"the workers' lock could not be taken");
}

static void crew_unlock(void)
{
	VOE_BASE_ASSERT(mtx_unlock(&crew.lock) == thrd_success,
			"the workers' lock could not be given back");
}

uint32_t voe_platform_burst_workers(bool splash)
{
	uint32_t cores = voe_platform_cores();
	uint32_t workers = splash ? cores - 1 : cores / 4;

	return workers < 1 ? 1 : workers;
}

// Takes the next index until none is left; every take is bounded by count.
// The thread's level is the burst's while its tasks run.
static void burst_run(voe_platform_burst *burst)
{
	uint32_t outer = task_level;

	VOE_BASE_ASSERT(burst != NULL, "no burst to run");
	VOE_BASE_ASSERT(burst->level >= 1 && burst->level <= 2,
			"a burst is outer or nested once");
	task_level = burst->level;
	for (;;) {
		uint64_t index = atomic_fetch_add_explicit(&burst->next, 1,
							   memory_order_relaxed);

		if (index >= burst->count)
			break;
		burst->task(burst->context, (uint32_t)index);
		atomic_fetch_add_explicit(&burst->ended, 1, memory_order_release);
	}
	task_level = outer;
}

// The oldest burst a worker may join, or NULL. Under the lock.
static voe_platform_burst *burst_oldest_open(void)
{
	voe_platform_burst *burst = crew.oldest;

	while (burst != NULL &&
	       (burst->room == 0 ||
		atomic_load_explicit(&burst->next, memory_order_relaxed) >=
			burst->count))
		burst = burst->later;
	return burst;
}

// A standing worker: serves bursts until close stops it. The loop is bounded
// by that close, which ends every open.
static int worker_serve(void *argument)
{
	VOE_BASE_ASSERT(argument == NULL, "a worker is given nothing");
	VOE_BASE_ASSERT(task_level == 0, "a worker starts in no task");
	crew_lock();
	for (;;) {
		voe_platform_burst *burst = burst_oldest_open();

		if (burst == NULL && crew.stopping)
			break;
		if (burst == NULL) {
			VOE_BASE_ASSERT(cnd_wait(&crew.work, &crew.lock) ==
						thrd_success,
					"a worker could not sleep");
			continue;
		}
		burst->room--;
		burst->working++;
		crew_unlock();
		burst_run(burst);
		crew_lock();
		burst->working--;
		if (burst->working == 0)
			VOE_BASE_ASSERT(cnd_broadcast(&crew.left) == thrd_success,
					"a finish could not be woken");
	}
	crew_unlock();
	return 0;
}

void voe_platform_workers_open(void)
{
	uint32_t cores = voe_platform_cores();
	uint32_t wanted = cores < 2 ? 2 : cores;

	crew_lock();
	if (crew.standing == 0) {
		crew.threads = calloc(wanted, sizeof(crew.threads[0]));
		VOE_BASE_ASSERT(crew.threads != NULL, "no room for the workers");
		while (crew.standing < wanted &&
		       thrd_create(&crew.threads[crew.standing], worker_serve,
				   NULL) == thrd_success)
			crew.standing++;
		if (crew.standing == 0) {
			free(crew.threads);
			crew.threads = NULL;
		}
	}
	VOE_BASE_ASSERT(crew.standing <= wanted, "never more workers than asked");
	crew_unlock();
}

void voe_platform_workers_close(void)
{
	thrd_t *threads;
	uint32_t standing;

	VOE_BASE_ASSERT(task_level == 0, "a task closes no workers");
	crew_lock();
	VOE_BASE_ASSERT(crew.oldest == NULL, "a burst is unfinished at close");
	threads = crew.threads;
	standing = crew.standing;
	crew.stopping = true;
	VOE_BASE_ASSERT(cnd_broadcast(&crew.work) == thrd_success,
			"the workers could not be woken to stop");
	crew_unlock();
	for (uint32_t i = 0; i < standing; i++)
		VOE_BASE_ASSERT(thrd_join(threads[i], NULL) == thrd_success,
				"a worker could not be joined");
	crew_lock();
	free(threads);
	crew.threads = NULL;
	crew.standing = 0;
	crew.stopping = false;
	crew_unlock();
}

uint32_t voe_platform_workers_standing(void)
{
	uint32_t standing;

	crew_lock();
	standing = crew.standing;
	crew_unlock();
	return standing;
}

// Appends the burst to the queue, newest last, and wakes a worker for each
// place it has room for. Under the lock.
static void burst_queue(voe_platform_burst *burst)
{
	voe_platform_burst *last = crew.oldest;

	VOE_BASE_ASSERT(burst->later == NULL, "a burst is queued once");
	if (last == NULL)
		crew.oldest = burst;
	while (last != NULL && last->later != NULL)
		last = last->later;
	if (last != NULL)
		last->later = burst;
	for (uint32_t i = 0; i < burst->room && i < crew.standing; i++)
		VOE_BASE_ASSERT(cnd_signal(&crew.work) == thrd_success,
				"a worker could not be woken");
}

voe_platform_burst *voe_platform_burst_start(uint32_t workers, uint32_t count,
					     voe_platform_burst_task *task,
					     void *context)
{
	voe_platform_burst *burst;
	uint32_t room = 0;

	VOE_BASE_ASSERT(workers >= 1, "a burst needs at least one worker");
	VOE_BASE_ASSERT(task != NULL, "a burst needs a task");
	VOE_BASE_ASSERT(task_level < 2, "a task of a nested burst starts no burst");
	voe_platform_workers_open();
	burst = calloc(1, sizeof(*burst));
	VOE_BASE_ASSERT(burst != NULL, "no room for a burst");
	burst->task = task;
	burst->context = context;
	burst->count = count;
	burst->level = task_level + 1;
	atomic_init(&burst->next, 0);
	atomic_init(&burst->ended, 0);

	crew_lock();
	if (crew.standing > 0)
		room = workers < count ? workers : count;
	burst->room = room;
	burst_queue(burst);
	crew_unlock();
	if (room == 0)
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
	voe_platform_burst *before;

	VOE_BASE_ASSERT(burst != NULL, "finish asked of no burst");
	crew_lock();
	while (!voe_platform_burst_done(burst) || burst->working > 0)
		VOE_BASE_ASSERT(cnd_wait(&crew.left, &crew.lock) == thrd_success,
				"a finish could not sleep");
	before = crew.oldest;
	if (before == burst)
		crew.oldest = burst->later;
	while (before != burst && before->later != burst)
		before = before->later;
	if (before != burst)
		before->later = burst->later;
	crew_unlock();
	free(burst);
}
