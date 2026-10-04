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
//
// THE LAUNCH IS FIELD 22 OF /proc/self/stat: the process's start in clock ticks
// since boot. Boot time counts a suspend and the monotonic clock does not, so
// the start is taken off a CLOCK_BOOTTIME reading and that age off `now`. The
// fields are counted from after the last ')', since the program's name in
// brackets before it may itself hold spaces or brackets.
#define _POSIX_C_SOURCE 199309L

#include <platform/clock.h>

#include <base/assert.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

// Field 22 of /proc/self/stat, in clock ticks since boot. False when the
// file cannot be read or does not have the shape the kernel documents.
static bool read_start_ticks(unsigned long long *ticks)
{
	char stat[4096];
	int fd = open("/proc/self/stat", O_RDONLY);
	ssize_t got;
	char *field;
	char *end;
	int spaces = 0;

	VOE_BASE_ASSERT(ticks != nullptr, "read_start_ticks needs somewhere to put it");
	if (fd < 0)
		return false;
	got = read(fd, stat, sizeof(stat) - 1);
	close(fd);
	if (got <= 0)
		return false;
	stat[got] = '\0';

	// After the last ')' comes " S ...", field 3 onward; field 22 is
	// nineteen spaces past field 3's start.
	field = strrchr(stat, ')');
	if (field == nullptr || field[1] != ' ')
		return false;
	field += 2;
	while (*field != '\0' && spaces < 19) {
		if (*field == ' ')
			spaces++;
		field++;
	}
	if (spaces < 19 || *field < '0' || *field > '9')
		return false;
	*ticks = strtoull(field, &end, 10);
	VOE_BASE_ASSERT(end > field, "a digit was checked for, so one was read");
	return true;
}

bool voe_platform_clock_launched(double *at)
{
	unsigned long long ticks;
	long per_second = sysconf(_SC_CLK_TCK);
	struct timespec boot;
	double booted;
	double now;

	VOE_BASE_ASSERT(at != nullptr, "voe_platform_clock_launched needs somewhere to put it");
	if (per_second <= 0 || !read_start_ticks(&ticks))
		return false;
	if (clock_gettime(CLOCK_BOOTTIME, &boot) != 0)
		return false;
	now = voe_platform_clock_now();
	booted = (double)boot.tv_sec + (double)boot.tv_nsec / 1e9;

	*at = now - (booted - (double)ticks / (double)per_second);
	VOE_BASE_ASSERT(*at <= now + 1.0, "a process cannot start after now by more than a tick's slack");
	return true;
}
