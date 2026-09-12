// The clock's arithmetic, driven with numbers chosen here rather than read off
// the machine. Needs no window and no graphics card — which is the whole reason
// voe_app_clock_tick takes the reading instead of making one.
//
// THE CLAMPED CASE IS WHAT THIS FILE IS FOR. `elapsed` and `step` are equal on
// every frame that went normally, so a mistake that returned one where the other
// was meant would pass every test but that one, and would then show up as
// physics going through a wall after a breakpoint.
#include <app/clock.h>

#include <testing/test.h>

// Comparing doubles that the code under test only subtracted: exact would pass,
// and this leaves room for the arithmetic to be rearranged later.
#define NEARLY 1e-12

static void check_the_first_tick_has_no_interval(void)
{
	voe_app_clock clock = { 0 };
	voe_app_tick tick = voe_app_clock_tick(&clock, 100.0, 0.25);

	VOE_TEST_CHECK(tick.first);
	VOE_TEST_CHECK_FLOAT(tick.now, 100.0, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 0.0, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.step, 0.0, NEARLY);
}

static void check_an_ordinary_interval(void)
{
	voe_app_clock clock = { 0 };
	voe_app_tick tick;

	(void)voe_app_clock_tick(&clock, 100.0, 0.25);

	tick = voe_app_clock_tick(&clock, 100.016, 0.25);
	VOE_TEST_CHECK(!tick.first);
	VOE_TEST_CHECK_FLOAT(tick.now, 100.016, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 0.016, NEARLY);
	// Under the ceiling, so the two are the same number.
	VOE_TEST_CHECK_FLOAT(tick.step, 0.016, NEARLY);

	// And the clock carries on from the second reading, not the first.
	tick = voe_app_clock_tick(&clock, 100.032, 0.25);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 0.016, NEARLY);
}

static void check_a_stall_is_clamped(void)
{
	voe_app_clock clock = { 0 };
	voe_app_tick tick;

	(void)voe_app_clock_tick(&clock, 100.0, 0.25);

	// Two seconds gone — a breakpoint, or a swapchain rebuilt. What really
	// happened is reported in full; what the world is stepped by is not.
	tick = voe_app_clock_tick(&clock, 102.0, 0.25);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 2.0, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.step, 0.25, NEARLY);

	// And the stall does not persist: the next ordinary frame is ordinary.
	tick = voe_app_clock_tick(&clock, 102.016, 0.25);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 0.016, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.step, 0.016, NEARLY);
}

static void check_two_ticks_at_the_same_reading(void)
{
	voe_app_clock clock = { 0 };
	voe_app_tick tick;

	(void)voe_app_clock_tick(&clock, 100.0, 0.25);

	// A clock too coarse to tell two frames apart. Zero, not the ceiling and
	// not a divide waiting to happen.
	tick = voe_app_clock_tick(&clock, 100.0, 0.25);
	VOE_TEST_CHECK(!tick.first);
	VOE_TEST_CHECK_FLOAT(tick.elapsed, 0.0, NEARLY);
	VOE_TEST_CHECK_FLOAT(tick.step, 0.0, NEARLY);
}

int main(void)
{
	check_the_first_tick_has_no_interval();
	check_an_ordinary_interval();
	check_a_stall_is_clamped();
	check_two_ticks_at_the_same_reading();
	return voe_test_result();
}
