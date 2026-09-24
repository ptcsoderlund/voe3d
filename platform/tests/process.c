// What platform/process.h promises, checked from outside: an exit code of zero
// and of non-zero comes back through a poll, a program still running is RUNNING
// and _end stops it promptly and zeroes the struct, a name that is not a
// program is a false start, and an output path collects stdout and stderr and
// is appended to by the next start.
//
// The programs are `cmake -E ...`, because cmake is on PATH wherever this engine
// builds, on both platforms, and `-E true`, `-E false` and `-E sleep` behave the
// same on each. Polling spins against platform/clock.h with a deadline, so a
// poll that never says ENDED fails the test rather than hanging it.
//
// THE OUTPUT CASE IS LINUX ONLY: it runs `sh -c`, which Windows does not have,
// in a mkdtemp folder under the working directory (the build tree under ctest),
// removed at the end. Tests here may include OS headers (check.cmake step 5);
// _GNU_SOURCE is what makes <stdlib.h> declare mkdtemp under -std=c23.
#define _GNU_SOURCE
#include <platform/clock.h>
#include <platform/process.h>

#include <platform/file.h>

#include <base/arena.h>
#include <testing/test.h>

#ifndef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

// Far longer than cmake takes to start and exit on any machine this runs on.
#define DEADLINE_SECONDS 30.0

// Polls until ENDED or the deadline; returns the exit code, or -1000 on timeout.
static int exit_code_of(const char *const *argv, const char *output, voe_base_arena *scratch)
{
	voe_platform_process process = { 0 };
	double start = voe_platform_clock_now();
	int code = 0;

	if (!voe_platform_process_start(argv, output, scratch, &process))
		return -1000;
	while (voe_platform_process_poll(&process, &code) == VOE_PLATFORM_PROCESS_RUNNING) {
		if (voe_platform_clock_now() - start > DEADLINE_SECONDS) {
			voe_platform_process_end(&process);
			return -1000;
		}
	}
	VOE_TEST_CHECK(process.handle == 0 && process.job == 0);
	return code;
}

static void test_exit_codes(voe_base_arena *scratch)
{
	const char *const succeeds[] = { "cmake", "-E", "true", NULL };
	const char *const fails[] = { "cmake", "-E", "false", NULL };
	int code = exit_code_of(fails, NULL, scratch);

	VOE_TEST_CHECK_INT(exit_code_of(succeeds, NULL, scratch), 0);
	VOE_TEST_CHECK(code != 0 && code != -1000);
}

static void test_end_stops_a_running_program(voe_base_arena *scratch)
{
	const char *const sleeps[] = { "cmake", "-E", "sleep", "30", NULL };
	voe_platform_process process = { 0 };
	int code = 0;
	double start;

	VOE_TEST_CHECK(voe_platform_process_start(sleeps, NULL, scratch, &process));
	if (process.handle == 0)
		return;
	VOE_TEST_CHECK_INT(voe_platform_process_poll(&process, &code), VOE_PLATFORM_PROCESS_RUNNING);
	start = voe_platform_clock_now();
	voe_platform_process_end(&process);
	VOE_TEST_CHECK(voe_platform_clock_now() - start < 5.0);
	VOE_TEST_CHECK(process.handle == 0 && process.job == 0);
}

static void test_missing_program_is_a_false_start(voe_base_arena *scratch)
{
	const char *const missing[] = { "voe-no-such-program-anywhere", NULL };
	voe_platform_process process = { 0 };

	VOE_TEST_CHECK(!voe_platform_process_start(missing, NULL, scratch, &process));
	VOE_TEST_CHECK(process.handle == 0);
}

#ifndef _WIN32
static void test_output_collects_both_streams_and_appends(voe_base_arena *scratch)
{
	const char *const prints[] = { "sh", "-c", "echo out; echo err >&2", NULL };
	char folder[] = "process-output-XXXXXX";
	char path[sizeof(folder) + 16];
	const uint8_t *text;
	size_t count;

	VOE_TEST_CHECK(mkdtemp(folder) != NULL);
	snprintf(path, sizeof(path), "%s/out.log", folder);

	VOE_TEST_CHECK_INT(exit_code_of(prints, path, scratch), 0);
	text = voe_platform_file_read(path, scratch, &count, NULL);
	VOE_TEST_CHECK(text != NULL && strcmp((const char *)text, "out\nerr\n") == 0);

	VOE_TEST_CHECK_INT(exit_code_of(prints, path, scratch), 0);
	text = voe_platform_file_read(path, scratch, &count, NULL);
	VOE_TEST_CHECK(text != NULL && strcmp((const char *)text, "out\nerr\nout\nerr\n") == 0);

	(void)remove(path);
	(void)rmdir(folder);
}
#endif

int main(void)
{
	voe_base_arena *scratch = voe_base_arena_new(4096);

	test_exit_codes(scratch);
	test_end_stops_a_running_program(scratch);
	test_missing_program_is_a_false_start(scratch);
#ifndef _WIN32
	test_output_collects_both_streams_and_appends(scratch);
#endif
	voe_base_arena_destroy(scratch);
	return voe_test_result();
}
