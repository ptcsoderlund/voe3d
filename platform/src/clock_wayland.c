// The Linux clock: CLOCK_MONOTONIC, in seconds. Read platform/clock.h first —
// what the number means and why it is not the time of day is written there.
//
// LINUX AND NOT WAYLAND, DESPITE THE NAME, AND THE NAME IS THE BUILD'S. A source
// whose name ends in _wayland is compiled on Linux only and nowhere else, which
// is the whole of what this file needs; there is no _linux suffix in
// cmake/voe.cmake and inventing one to be tidy would put a second rule in the
// build for the sake of one file. platform/src/library_wayland.c carries the
// same name for the same reason. Nothing in here knows there is a compositor.
//
// _POSIX_C_SOURCE is what makes <time.h> declare clock_gettime under -std=c23,
// which the engine builds with. It is a feature-test macro and not a use of any
// extension; 199309L is the revision that added the monotonic clock.
//
// CLOCK_MONOTONIC AND NOT CLOCK_MONOTONIC_RAW. The raw one is not adjusted by
// NTP's slewing, which sounds like the more honest of the two and is the wrong
// one here: slewing is how the machine's idea of a second is corrected towards a
// real second, so the raw clock measures a frame in ticks of a slightly wrong
// second. Neither ever jumps or runs backwards, which is the property this file
// actually needs, and the adjusted one is the one that agrees with a stopwatch.
#define _POSIX_C_SOURCE 199309L

#include <platform/clock.h>

#include <base/assert.h>

#include <time.h>

double voe_platform_clock_now(void)
{
	struct timespec now;
	int asked = clock_gettime(CLOCK_MONOTONIC, &now);

	// A failure here means CLOCK_MONOTONIC is not a clock this kernel has,
	// which no Linux the engine supports can be. It is the program asking
	// for something that cannot fail rather than the world refusing
	// something, so it aborts rather than handing back a number that would
	// quietly be nought — a clock stuck at zero reads as a frame loop that
	// takes no time at all, which is the worst way for this to go wrong.
	VOE_BASE_ASSERT(asked == 0, "this kernel has no monotonic clock");

	return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
}
