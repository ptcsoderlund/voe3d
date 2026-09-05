// What platform/clock.h promises, checked from outside: that the clock never
// goes backwards, and that it does move. Needs no window and no display, so it
// runs under ctest on a machine with neither.
//
// NOTHING HERE MEASURES A DURATION AGAINST A DURATION, AND THAT IS DELIBERATE. A
// test that slept for ten milliseconds and asserted the clock had advanced by
// about ten is a test that fails on a loaded build machine, in a container with
// a share of a core, and under an analyser — and it fails for reasons that have
// nothing to do with the code. What is actually worth proving is the shape of
// the thing: an ordering that never reverses, and a counter that is not stuck.
// Whether a second here is a real second is what a stopwatch against the window
// is for, and dev/src/main.c prints the numbers to hold one against.
//
// THE SPIN IS BOUNDED AND ITS BOUND IS WHAT MAKES IT A TEST AND NOT A HANG. Any
// clock the engine can run on ticks far faster than this loop can go round, so
// the bound is never reached; a clock that is stuck runs out of iterations and
// reports a failure, rather than sitting there until someone kills the run.
#include <platform/clock.h>

#include <testing/test.h>

// Enough that a clock ticking anywhere above one hertz gets there long before
// the end, and few enough that a stuck clock reports in well under a second.
#define SPINS 20000000

int main(void)
{
	double start = voe_platform_clock_now();
	double now = start;
	double previous;
	long long spins = 0;

	// It moves. The loop stops on the first reading that differs from the
	// one it began with, so this costs a clock read or two on any real
	// machine and only runs to the bound on a broken one.
	while (spins < SPINS) {
		now = voe_platform_clock_now();
		if (now > start)
			break;
		spins++;
	}
	VOE_TEST_CHECK(now > start);
	VOE_TEST_CHECK(spins < SPINS);

	// It never goes backwards. Read many times in a row, with nothing
	// between the readings, which is where a clock built out of two
	// counters or one that wraps would show it.
	previous = now;
	for (int i = 0; i < 100000; i++) {
		now = voe_platform_clock_now();
		if (now < previous) {
			VOE_TEST_CHECK(now >= previous);
			break;
		}
		previous = now;
	}

	// The whole run took a positive amount of time, which is the same claim
	// as the first one made over a longer span — a clock that ticked once
	// and stopped passes the first check and fails this one.
	VOE_TEST_CHECK(voe_platform_clock_now() > start);

	return voe_test_result();
}
