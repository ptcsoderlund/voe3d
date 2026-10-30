// What platform/burst.h promises, checked from outside: the core count is at
// least 1 and kept, both worker counts are at least 1 and below it, every task
// of a burst runs exactly once whatever the workers, and done turns true and
// stays; bursts reuse the standing workers without the count moving, a task
// runs a nested burst to its end, and close ends them until a start reopens
// them. Needs no window and no display.
//
// Each task adds one to its own index's counter and touches nothing else, as
// the header asks, so a counter of 1 everywhere is "each ran once". The wait
// for done is bounded: a burst that never ends fails after ten seconds rather
// than hanging the run.
#include <platform/burst.h>

#include <testing/test.h>

#include <stdlib.h>
#include <threads.h>

// Milliseconds the wait for done gives a burst before calling it stuck.
#define PATIENCE 10000

static void count_once(void *context, uint32_t index)
{
	uint32_t *ran = context;

	ran[index]++;
}

// Runs `count` tasks on `workers`, waiting on done; every index ran once.
static void each_runs_once(uint32_t workers, uint32_t count)
{
	uint32_t *ran = calloc(count == 0 ? 1 : count, sizeof(*ran));
	voe_platform_burst *burst = voe_platform_burst_start(workers, count, count_once, ran);
	int waited = 0;
	uint32_t wrong = 0;

	while (!voe_platform_burst_done(burst) && waited < PATIENCE) {
		(void)thrd_sleep(&(struct timespec){ .tv_nsec = 1000000 }, NULL);
		waited++;
	}
	VOE_TEST_CHECK(voe_platform_burst_done(burst));
	VOE_TEST_CHECK(voe_platform_burst_done(burst));
	voe_platform_burst_finish(burst);
	for (uint32_t i = 0; i < count; i++)
		wrong += ran[i] != 1;
	VOE_TEST_CHECK_INT(wrong, 0);
	free(ran);
}

static void cores_and_workers(void)
{
	uint32_t cores = voe_platform_cores();
	uint32_t splash = voe_platform_burst_workers(true);
	uint32_t running = voe_platform_burst_workers(false);

	VOE_TEST_CHECK(cores >= 1);
	VOE_TEST_CHECK_INT(voe_platform_cores(), cores);
	VOE_TEST_CHECK(splash >= 1);
	VOE_TEST_CHECK(running >= 1);
	if (cores > 1) {
		VOE_TEST_CHECK(splash < cores);
		VOE_TEST_CHECK(running < cores);
	}
}

// No tasks: done before anything could have run.
static void empty_is_done_at_once(void)
{
	voe_platform_burst *burst = voe_platform_burst_start(4, 0, count_once, NULL);

	VOE_TEST_CHECK(voe_platform_burst_done(burst));
	voe_platform_burst_finish(burst);
}

static void record_thread(void *context, uint32_t index)
{
	thrd_t *ran_on = context;

	ran_on[index] = thrd_current();
}

// Adds each recorded thread not yet in `seen` to it; at most `room` kept, and
// room + 1 once there are more.
static uint32_t threads_add(thrd_t *seen, uint32_t known, uint32_t room,
			    const thrd_t *ran_on, uint32_t count)
{
	for (uint32_t i = 0; i < count && known <= room; i++) {
		uint32_t j = 0;

		while (j < known && !thrd_equal(seen[j], ran_on[i]))
			j++;
		if (j == known && known < room)
			seen[known++] = ran_on[i];
		else if (j == known)
			return room + 1;
	}
	return known;
}

// 50 bursts on the open workers: the standing count never moves, and every
// task ran on one of at most that many threads.
static void bursts_reuse_the_standing_workers(void)
{
	enum { BURSTS = 50, TASKS = 64 };
	thrd_t ran_on[TASKS];
	thrd_t *seen;
	uint32_t standing, known = 0, moved = 0;

	voe_platform_workers_open();
	standing = voe_platform_workers_standing();
	VOE_TEST_CHECK(standing >= 1);
	seen = calloc(standing, sizeof(*seen));
	for (uint32_t b = 0; b < BURSTS; b++) {
		voe_platform_burst *burst = voe_platform_burst_start(
			voe_platform_burst_workers(true), TASKS, record_thread, ran_on);

		voe_platform_burst_finish(burst);
		moved += voe_platform_workers_standing() != standing;
		known = threads_add(seen, known, standing, ran_on, TASKS);
	}
	voe_platform_workers_open();
	VOE_TEST_CHECK_INT(voe_platform_workers_standing(), standing);
	VOE_TEST_CHECK_INT(moved, 0);
	VOE_TEST_CHECK(known >= 1 && known <= standing);
	free(seen);
}

enum { INNER_TASKS = 200 };

// The outer task: a nested burst as wide as a splash's, started and finished.
static void start_inner(void *context, uint32_t index)
{
	uint32_t *ran = context;
	voe_platform_burst *inner = voe_platform_burst_start(
		voe_platform_burst_workers(true), INNER_TASKS, count_once, ran);

	(void)index;
	voe_platform_burst_finish(inner);
}

// A task of a one-worker burst runs a burst of its own to the end.
static void task_runs_a_nested_burst(void)
{
	uint32_t ran[INNER_TASKS] = { 0 };
	uint32_t wrong = 0;
	voe_platform_burst *outer = voe_platform_burst_start(1, 1, start_inner, ran);

	voe_platform_burst_finish(outer);
	for (uint32_t i = 0; i < INNER_TASKS; i++)
		wrong += ran[i] != 1;
	VOE_TEST_CHECK_INT(wrong, 0);
}

// Close ends every worker; a later start opens them again.
static void close_then_start_reopens(void)
{
	voe_platform_workers_close();
	VOE_TEST_CHECK_INT(voe_platform_workers_standing(), 0);
	voe_platform_workers_close();
	each_runs_once(4, 100);
	VOE_TEST_CHECK(voe_platform_workers_standing() >= 1);
	voe_platform_workers_close();
	VOE_TEST_CHECK_INT(voe_platform_workers_standing(), 0);
}

int main(void)
{
	cores_and_workers();
	each_runs_once(4, 10000);
	empty_is_done_at_once();
	each_runs_once(8, 3);
	bursts_reuse_the_standing_workers();
	task_runs_a_nested_burst();
	close_then_start_reopens();
	return voe_test_result();
}
