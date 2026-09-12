// The interval between two frames, as a value. One function: hand it the
// reading you took and it hands back how long the frame was and how long to
// step the world.
//
//     voe_app_clock clock = { 0 };
//     while (running) {
//             voe_app_tick tick = voe_app_clock_tick(&clock,
//                                                    voe_platform_clock_now(),
//                                                    0.25);
//             world_advance(tick.step);
//     }
//
// IT TAKES THE READING RATHER THAN MAKING ONE, and that is the whole reason it
// is testable. Nothing in here calls platform, so the test below it drives a
// clock with numbers it chose and asserts on the arithmetic; a clock that read
// the machine could only be tested by waiting. The caller passing
// voe_platform_clock_now() is the ordinary use and the only one the engine has.
//
// `elapsed` AND `step` ARE DIFFERENT NUMBERS AND CONFUSING THEM IS THE BUG THIS
// SPLIT EXISTS TO PREVENT. `elapsed` is what really happened and is what a
// timing readout should say. `step` is `elapsed` with a ceiling on it and is
// what anything integrating over time should be given. They are the same number
// on a frame that went normally and they part company exactly when something
// stalled the program — a breakpoint, a window dragged across a monitor, a
// driver taking a second to rebuild a swapchain. Stepping physics by the real
// two seconds that took puts everything through a wall; stepping by the ceiling
// runs the world slow for one frame, which nobody notices.
//
// THE FIRST TICK HAS NO INTERVAL AND SAYS SO. There is no previous reading to
// subtract, so `elapsed` and `step` are zero and `first` is true. A zeroed
// struct is a clock that has not started; there is no _init, and there is
// nothing to destroy.
//
// It does not accumulate, it does not average and it does not count frames. A
// readout that wants any of those keeps its own numbers from `elapsed`.
#pragma once

// A zeroed one has not ticked yet. Both fields are this module's; read `now`
// off the tick rather than out of here.
typedef struct {
	double previous;
	bool started;
} voe_app_clock;

// `now` is the reading that produced this tick, passed back so a caller holding
// the tick does not need a second one. Seconds, from platform's monotonic clock.
typedef struct {
	double now;
	double elapsed;
	double step;
	bool first;
} voe_app_tick;

// `longest_step` is the ceiling on `step`, in seconds, and must be above zero —
// at or below is the caller's bug and asserts. It is a parameter rather than a
// field because it is the program's policy and not the clock's: a program may
// pass a different one on a frame it knows was slow for a reason of its own.
voe_app_tick voe_app_clock_tick(voe_app_clock *clock, double now,
				double longest_step);
