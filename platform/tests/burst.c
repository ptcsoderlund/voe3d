// What platform/burst.h promises, checked from outside: the core count is at
// least 1 and kept, both worker counts are at least 1 and below it, every task
// of a burst runs exactly once whatever the workers, and done turns true and
// stays. Needs no window and no display.
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

int main(void)
{
	cores_and_workers();
	each_runs_once(4, 10000);
	empty_is_done_at_once();
	each_runs_once(8, 3);
	return voe_test_result();
}
