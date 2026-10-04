// The start log: steps come back in order and add up, the file keeps one start
// after another, and a file grown past the limit is emptied before the new
// block. Needs no window and no graphics card.
//
// The files are written into the working directory ctest runs the test in, as
// capture.c writes its picture, and removed at the end, pass or fail.
#include <app/start_log.h>

#include <base/arena.h>
#include <platform/file.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_PATH "app_start_log_test.log"
#define TOTAL_LINE "test: total "
#define NEARLY 1e-12

static void steps_in_order_add_up(void)
{
	voe_app_start_log log;
	size_t first;
	double sum = 0.0;

	voe_app_start_log_begin(&log);
	voe_app_start_log_step(&log, "window");
	voe_app_start_log_step(&log, "pipelines");

	// "before main" only where the OS can say when it started us.
	first = log.count == 3 ? 1 : 0;
	VOE_TEST_CHECK_INT(log.count, first + 2);
	if (first == 1)
		VOE_TEST_CHECK(strcmp(log.steps[0].name, "before main") == 0);
	VOE_TEST_CHECK(strcmp(log.steps[first].name, "window") == 0);
	VOE_TEST_CHECK(strcmp(log.steps[first + 1].name, "pipelines") == 0);

	for (size_t i = 0; i < log.count; i++) {
		VOE_TEST_CHECK(log.steps[i].seconds >= 0.0);
		sum += log.steps[i].seconds;
	}
	VOE_TEST_CHECK_FLOAT(voe_app_start_log_total(&log), sum, NEARLY);
}

// How many times needle is in the file's text.
static int count_in_file(const char *needle, voe_base_arena *arena,
			 size_t *length)
{
	const char *text = (const char *)voe_platform_file_read(LOG_PATH, arena,
								length, NULL);
	int found = 0;

	if (text == NULL)
		return -1;
	for (const char *at = strstr(text, needle); at != NULL;
	     at = strstr(at + 1, needle))
		found++;
	return found;
}

static void file_holds_both_starts(voe_base_arena *arena)
{
	voe_app_start_log log;
	size_t length;

	remove(LOG_PATH);
	voe_app_start_log_begin(&log);
	voe_app_start_log_step(&log, "window");
	VOE_TEST_CHECK(voe_app_start_log_write(&log, "test", LOG_PATH, arena));
	VOE_TEST_CHECK(voe_app_start_log_write(&log, "test", LOG_PATH, arena));
	VOE_TEST_CHECK_INT(count_in_file(TOTAL_LINE, arena, &length), 2);
}

static void file_over_limit_is_emptied(voe_base_arena *arena)
{
	static uint8_t filler[65 * 1024];
	voe_app_start_log log;
	size_t length;
	size_t block;

	memset(filler, 'x', sizeof(filler));
	VOE_TEST_CHECK(voe_platform_file_write(LOG_PATH, filler,
					       sizeof(filler), NULL));
	voe_app_start_log_begin(&log);
	voe_app_start_log_step(&log, "window");
	VOE_TEST_CHECK(voe_app_start_log_write(&log, "test", LOG_PATH, arena));

	VOE_TEST_CHECK_INT(count_in_file(TOTAL_LINE, arena, &length), 1);
	VOE_TEST_CHECK_INT(count_in_file("x", arena, &block), 0);
	VOE_TEST_CHECK(length < 1024);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);

	steps_in_order_add_up();
	file_holds_both_starts(arena);
	file_over_limit_is_emptied(arena);

	voe_base_arena_destroy(arena);
	remove(LOG_PATH);
	return voe_test_result();
}
