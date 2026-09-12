// The subtraction and the ceiling, and nothing else. The reasoning is in
// app/include/app/clock.h.
#include <app/clock.h>

#include <base/assert.h>

#include <stddef.h>

voe_app_tick voe_app_clock_tick(voe_app_clock *clock, double now,
				double longest_step)
{
	voe_app_tick tick = { 0 };

	VOE_BASE_ASSERT(clock != NULL, "a tick needs a clock");
	VOE_BASE_ASSERT(longest_step > 0.0,
			"longest_step is a ceiling and has to be above zero");

	tick.now = now;
	tick.first = !clock->started;

	// Left at zero on the first tick: there is no previous reading, so the
	// interval is not small, it does not exist. See the header.
	if (clock->started) {
		tick.elapsed = now - clock->previous;
		tick.step = tick.elapsed > longest_step ? longest_step
						       : tick.elapsed;
	}

	clock->previous = now;
	clock->started = true;
	return tick;
}
