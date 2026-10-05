// The Windows clock: the performance counter, in seconds. Read platform/clock.h
// first — what the number means and why it is not the time of day is written
// there.
//
// QueryPerformanceCounter AND NOT GetTickCount OR timeGetTime. The tick counts
// are milliseconds, and a millisecond is a fifteenth of a frame at sixty hertz:
// a frame loop measured in them reports a handful of distinct numbers and no
// stutter smaller than a whole millisecond exists as far as it is concerned.
// The performance counter is the one Windows documents as monotonic, as
// consistent across processors, and as the one to time with.
//
// THE FREQUENCY IS ASKED FOR ONCE AND KEPT. Windows fixes it at boot and
// documents that it does not change while the system runs, so asking every time
// would be a system call to be told the same number. It is a file-scope static
// filled on the first call, which is sound here because this engine is
// single-threaded — the same standing platform's window and input already have.
//
// NEITHER CALL CAN FAIL ON ANYTHING THIS ENGINE RUNS ON. Both return non-zero on
// every version of Windows since XP and are documented never to fail; a zero is
// therefore the program running somewhere it cannot, not the world refusing
// something, so it aborts. A clock stuck at zero reads as a frame loop that takes
// no time at all, which is the worst way for this to go wrong.
#include <platform/clock.h>

#include <base/assert.h>

#include <windows.h>

static double ticks_per_second;

double voe_platform_clock_now(void)
{
	LARGE_INTEGER now;
	BOOL asked;

	if (ticks_per_second == 0.0) {
		LARGE_INTEGER frequency;

		asked = QueryPerformanceFrequency(&frequency);
		VOE_BASE_ASSERT(asked != 0,
				"this machine has no performance counter");
		VOE_BASE_ASSERT(frequency.QuadPart > 0,
				"the performance counter reports a frequency of nought");
		ticks_per_second = (double)frequency.QuadPart;
	}

	asked = QueryPerformanceCounter(&now);
	VOE_BASE_ASSERT(asked != 0, "the performance counter would not be read");

	return (double)now.QuadPart / ticks_per_second;
}

// THE LAUNCH IS THE PROCESS'S CREATION TIME, MOVED ONTO THIS CLOCK. Windows
// keeps it as a FILETIME, the time of day in hundreds of nanoseconds, so the age
// is the time of day now less that, and the launch is this clock's now less the
// age. A change to the time of day between the two would move it; nothing here
// needs better than the 10 ms Linux gives.
bool voe_platform_clock_launched(double *at)
{
	FILETIME created;
	FILETIME exited;
	FILETIME kernel;
	FILETIME user;
	FILETIME wall;
	ULARGE_INTEGER from;
	ULARGE_INTEGER to;
	double now;

	VOE_BASE_ASSERT(at != nullptr, "voe_platform_clock_launched needs somewhere to put it");
	if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
		return false;
	now = voe_platform_clock_now();
	GetSystemTimePreciseAsFileTime(&wall);
	from.LowPart = created.dwLowDateTime;
	from.HighPart = created.dwHighDateTime;
	to.LowPart = wall.dwLowDateTime;
	to.HighPart = wall.dwHighDateTime;
	if (to.QuadPart < from.QuadPart)
		return false;

	*at = now - (double)(to.QuadPart - from.QuadPart) / 1e7;
	return true;
}
