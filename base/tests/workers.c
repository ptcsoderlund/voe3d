// What base/workers.h promises, checked from outside: NULL and a pool of no
// threads run in order on the caller; a pool runs each index exactly once;
// runs nest, and two threads run on one pool at once, and all finish; a task
// reports under its caller's subjects and the lowest task's kept error
// becomes the caller's unless the caller kept its own; destroy returns.
//
// The check macros count failures in a plain static, so every check is made
// on the main thread: tasks and the test's own threads write flags and atomic
// sums, and main reads them once the run has returned or the thread joined.
#include <base/report.h>
#include <base/workers.h>

#include <testing/test.h>

#include <stdatomic.h>
#include <string.h>
#include <threads.h>

enum { MANY = 10000, OUTER = 50, INNER = 20 };

struct in_order {
	uint32_t order[100];
	uint32_t next;
};

static void note_order(void *context, uint32_t index)
{
	struct in_order *in_order = context;

	in_order->order[in_order->next++] = index;
}

static void check_in_order(voe_base_workers *workers)
{
	static struct in_order in_order;

	in_order.next = 0;
	voe_base_workers_run(workers, 100, note_order, &in_order);
	VOE_TEST_CHECK_INT(in_order.next, 100);
	for (uint32_t i = 0; i < 100; i++)
		VOE_TEST_CHECK_INT(in_order.order[i], i);
}

struct once {
	atomic_uint ran[MANY];
	atomic_ullong sum;
};

static void note_once(void *context, uint32_t index)
{
	struct once *once = context;

	atomic_fetch_add(&once->ran[index], 1);
	atomic_fetch_add(&once->sum, index);
}

// Each index ran exactly once; flags left as found for the next run.
static void check_once(struct once *once, uint32_t count)
{
	uint32_t wrong = 0;

	for (uint32_t i = 0; i < count; i++) {
		wrong += atomic_load(&once->ran[i]) != 1;
		atomic_store(&once->ran[i], 0);
	}
	VOE_TEST_CHECK_INT(wrong, 0);
	VOE_TEST_CHECK_INT((long long)atomic_load(&once->sum),
			   (long long)count * (count - 1) / 2);
	atomic_store(&once->sum, 0);
}

struct nested {
	voe_base_workers *workers;
	struct once once;
};

struct nested_outer {
	struct nested *nested;
	uint32_t outer;
};

static void run_inner(void *context, uint32_t index)
{
	struct nested_outer *outer = context;

	note_once(&outer->nested->once, outer->outer * INNER + index);
}

static void run_outer(void *context, uint32_t index)
{
	struct nested_outer outer = { context, index };

	voe_base_workers_run(outer.nested->workers, INNER, run_inner, &outer);
}

static void check_nested(uint32_t threads)
{
	static struct nested nested;

	nested.workers = voe_base_workers_new(threads);
	voe_base_workers_run(nested.workers, OUTER, run_outer, &nested);
	check_once(&nested.once, OUTER * INNER);
	voe_base_workers_destroy(nested.workers);
}

struct beside {
	voe_base_workers *workers;
	atomic_uint count;
};

static void count_one(void *context, uint32_t index)
{
	struct beside *beside = context;

	(void)index;
	atomic_fetch_add(&beside->count, 1);
}

static int run_beside(void *context)
{
	struct beside *beside = context;

	for (uint32_t i = 0; i < 20; i++)
		voe_base_workers_run(beside->workers, 500, count_one, beside);
	return 0;
}

static void check_two_callers(voe_base_workers *workers)
{
	static struct beside first;
	static struct beside second;
	thrd_t threads[2];

	first.workers = workers;
	second.workers = workers;
	VOE_TEST_CHECK(thrd_create(&threads[0], run_beside, &first) ==
		       thrd_success);
	VOE_TEST_CHECK(thrd_create(&threads[1], run_beside, &second) ==
		       thrd_success);
	VOE_TEST_CHECK(thrd_join(threads[0], NULL) == thrd_success);
	VOE_TEST_CHECK(thrd_join(threads[1], NULL) == thrd_success);
	VOE_TEST_CHECK_INT(atomic_load(&first.count), 20 * 500);
	VOE_TEST_CHECK_INT(atomic_load(&second.count), 20 * 500);
}

static void fail_three_and_seven(void *context, uint32_t index)
{
	(void)context;
	if (index == 3 || index == 7)
		VOE_BASE_ERROR("test", "task %u failed", (unsigned)index);
}

static void check_reports(voe_base_workers *workers)
{
	const char *kept;

	voe_base_report_subject_push("Assets/a.glb");
	voe_base_report_error_clear();
	voe_base_workers_run(workers, 10, fail_three_and_seven, NULL);
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL &&
		       strcmp(kept, "Assets/a.glb: task 3 failed") == 0);

	voe_base_report_error_clear();
	VOE_BASE_ERROR("test", "mine");
	voe_base_workers_run(workers, 10, fail_three_and_seven, NULL);
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL && strcmp(kept, "Assets/a.glb: mine") == 0);

	// The caller's own subjects are back: one pushed, one popped, none left.
	voe_base_report_subject_pop();
	voe_base_report_error_clear();
	VOE_BASE_ERROR("test", "after");
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL && strcmp(kept, "after") == 0);
}

int main(void)
{
	static struct once once;

	VOE_TEST_CHECK_INT(voe_base_workers_threads(NULL), 1);
	check_in_order(NULL);
	voe_base_workers *none = voe_base_workers_new(0);
	VOE_TEST_CHECK_INT(voe_base_workers_threads(none), 1);
	check_in_order(none);
	voe_base_workers_destroy(none);

	voe_base_workers *three = voe_base_workers_new(3);
	VOE_TEST_CHECK_INT(voe_base_workers_threads(three), 4);
	voe_base_workers_run(three, MANY, note_once, &once);
	check_once(&once, MANY);

	check_nested(1);
	check_nested(3);
	check_two_callers(three);
	check_reports(three);

	voe_base_workers_destroy(three);
	voe_base_workers_destroy(NULL);
	return voe_test_result();
}
