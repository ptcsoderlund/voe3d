// What platform/process.h promises, checked from outside: an exit code of zero
// and of non-zero comes back through a poll, a program still running is RUNNING
// and _end stops it promptly and zeroes the struct, and a name that is not a
// program is a false start.
//
// The programs are `cmake -E ...`, because cmake is on PATH wherever this engine
// builds, on both platforms, and `-E true`, `-E false` and `-E sleep` behave the
// same on each. Polling spins against platform/clock.h with a deadline, so a
// poll that never says ENDED fails the test rather than hanging it.
#include <platform/clock.h>
#include <platform/process.h>

#include <base/arena.h>
#include <testing/test.h>

// Far longer than cmake takes to start and exit on any machine this runs on.
#define DEADLINE_SECONDS 30.0

// Polls until ENDED or the deadline; returns the exit code, or -1000 on timeout.
static int exit_code_of(const char *const *argv, voe_base_arena *scratch)
{
	voe_platform_process process = { 0 };
	double start = voe_platform_clock_now();
	int code = 0;

	if (!voe_platform_process_start(argv, scratch, &process))
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
	int code = exit_code_of(fails, scratch);

	VOE_TEST_CHECK_INT(exit_code_of(succeeds, scratch), 0);
	VOE_TEST_CHECK(code != 0 && code != -1000);
}

static void test_end_stops_a_running_program(voe_base_arena *scratch)
{
	const char *const sleeps[] = { "cmake", "-E", "sleep", "30", NULL };
	voe_platform_process process = { 0 };
	int code = 0;
	double start;

	VOE_TEST_CHECK(voe_platform_process_start(sleeps, scratch, &process));
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

	VOE_TEST_CHECK(!voe_platform_process_start(missing, scratch, &process));
	VOE_TEST_CHECK(process.handle == 0);
}

int main(void)
{
	voe_base_arena *scratch = voe_base_arena_new(4096);

	test_exit_codes(scratch);
	test_end_stops_a_running_program(scratch);
	test_missing_program_is_a_false_start(scratch);
	voe_base_arena_destroy(scratch);
	return voe_test_result();
}
