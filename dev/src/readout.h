// What the loop measures, what it prints and what stands in the top-left of the
// view: the four durations and the two counts it keeps, the block printed every
// VOE_DEV_REPORT_SECONDS, and the same numbers laid out as text every frame.
//
// ITS OWN FILE BECAUSE IT IS THE MEASUREMENT AND NOT THE FRAME. main.c owns the
// loop and takes every sample into the struct below; this file owns the shape
// they are kept in and the two ways they are shown. The readout's entity is
// src/text.h's and where it stands is src/facing.h's; nothing here places
// anything.
//
// main.c CALLS THE THREE AT THREE POINTS: the legend once at startup, the
// readout between voe_app_draw_open and the draw system — the only place a
// transient block can be built — and the block when the period is over, which
// is main.c's own test on `started`.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <base/samples.h>
#include <ecs/world.h>
#include <platform/window.h>
#include <render/device.h>
#include <text/font.h>

#include <stdbool.h>
#include <stdint.h>

// How often the timing block is printed, in seconds. Each block is an average
// and a worst over exactly the period since the last one, so this is also the
// window every number in it is measured over. Two seconds is long
// enough to average a hundred frames and short enough that changing something
// and looking at the console is the same motion.
#define VOE_DEV_REPORT_SECONDS 2.0

// The readout: the same distance in front of the eye as the heads-up line,
// smaller than it, and snapped into the top-left corner of the view. Where the
// corner is in metres at that distance follows from the camera's field of view
// and the window's aspect ratio — see src/facing.h — so it stays in the corner
// when the window is resized.
#define VOE_DEV_READOUT_EM 0.040f

// The four numbers the loop measures, and the clock reading that says when a
// period ends. One struct because they are gathered together, reported together
// and reset together, and four loose pairs at the top of main.c's main() would
// be twelve variables to keep in step.
//
// AND IT PRINTS WHAT IT MEASURED. Four numbers every couple of seconds, each an
// average and a worst over exactly that period: the frame, this program's own
// work, the draw, and the graphics card's own clock. voe_dev_say_what_is_measured() is
// the legend and it is printed once at startup, because a number whose meaning
// is ambiguous is worse than no number. P switches between the two present
// modes, which is the measurement the frame-pacing decision is waiting on.
//
// EACH ONE BRACKETS EXACTLY ONE THING AND THE NAMES BELOW ARE THE WHOLE POINT. A
// single "frame time" would hide which of the four is the one that got longer,
// and that is the question a person is asking when they look at this at all.
struct voe_dev_timing {
	// One top of the loop to the next. Everything is inside it and its
	// average is what "frames per second" is the reciprocal of.
	voe_base_samples frame;
	// The poll, the input, and the three systems. This program's own work
	// before it asks the GPU for anything.
	voe_base_samples update;
	// The loop's draw phase, _begin to _end inclusive: waiting for the frame
	// slot, taking a swapchain image, laying out and uploading the readout,
	// recording every draw, submitting and presenting.
	voe_base_samples draw;
	// The graphics card's own clock, over that frame's commands only.
	voe_base_samples gpu;
	// The last full period's averages of the two numbers measured AFTER the
	// readout is built, and — for the card — whether it has ever answered.
	//
	// THEY EXIST BECAUSE THE PERIOD IS RESET WITH THE READOUT STILL
	// RUNNING, AND THEY ARE FOR THE READOUT AND NOT THE CONSOLE. `draw` and
	// `gpu` are sampled below voe_dev_build_the_readout in the loop, so for the one
	// frame after every console block their sample sets are empty while the
	// readout is being built — and voe_base_samples_average of nothing is
	// nought. Showing that nought is a draw phase that appears to have taken
	// no time at all, once every period, which is a number a person would
	// believe. `frame` and `update` need none of this: they are sampled
	// ABOVE voe_dev_build_the_readout, so they always hold at least this frame.
	bool gpu_timed;
	double gpu_last;
	double draw_last;
	// Draw commands per completed frame, averaged and worsted over the
	// period exactly as the four durations above are.
	//
	// IT IS A METRIC AND NOT A SNAPSHOT, WHICH IS THE PRINCIPAL'S CALL AND
	// REVERSES WHAT CARD 040 FIRST SAID. That card argued a draw count is
	// exact and that averaging it turns a true number into a smeared one.
	// That is right about this demo, where the scene is identical every
	// frame and the average is 32.0 for ever — and it is wrong about the
	// direction of travel. The moment anything varies what is drawn, culling
	// or streaming or an interface that grows, the number a person needs is
	// the WORST frame in the period rather than the typical one, which is
	// the same reason `frame` and `gpu` carry a worst beside their average.
	// A metric that only becomes interesting later is still the right shape
	// now; a snapshot that has to be replaced later is not.
	//
	// IT IS A voe_base_samples EVEN THOUGH A COUNT IS NOT A DURATION. That
	// type's real constraint is that `worst` is the LARGEST value, so a
	// quantity where small is bad does not belong in it. More draw commands
	// is worse, so this belongs. See base/include/base/samples.h, whose
	// header still says everything measured with one is a duration — true of
	// every other caller and no longer of this one.
	voe_base_samples draws;
	// What the readout shows for the one frame per period that has no sample
	// yet, and whether any frame has completed at all. The same three cases
	// `draw` and `gpu` have above and for the same reason: the period is
	// reset with the readout still running.
	bool draws_measured;
	double draws_last;
	double draws_last_worst;
	// And how many element records the last completed frame submitted, which
	// is the other half of ADR-0092's claim and the reason the pair is worth
	// showing.
	//
	// THIS ONE IS NOT AVERAGED AND THAT IS NOT AN OVERSIGHT. The claim is
	// "this many records cost that few commands", and a record count averaged
	// over a period no longer lines up with the commands beside it — one
	// would describe the period and the other a frame in it. The exact count
	// from the same completed frame keeps the pair a pair.
	//
	// THE GAP BETWEEN THE TWO IS THE CLAIM, AND NEITHER NUMBER SAYS IT
	// ALONE. Ninety-odd records and thirty-odd commands is "many small
	// things in one draw" stated rather than asserted; the commands alone
	// could be thirty-two of anything, and the records alone say nothing
	// about what they cost. See voe_render_frame_elements_submitted, whose
	// own header is where that gap is spelled out.
	uint32_t drawn_elements;
	// When this period began, on the same clock every sample is taken with.
	// Kept rather than a deadline, because the rate printed has to be over
	// the time the period really covered and a frame always straddles the
	// end of one.
	double started;
};

// The legend for the block, printed once at startup.
void voe_dev_say_what_is_measured(void);

// The readout's text this frame, out of what the period has measured so far,
// laid out through the transient path and put on the `readout` entity.
// `glyphs` comes back as how many characters were laid out. See readout.c for
// what each line says and what is wrong if it looks wrong.
[[nodiscard]] bool voe_dev_build_the_readout(
	voe_ecs_world *world, voe_render_device *gpu, const voe_text_font *font,
	voe_base_arena *arena, voe_ecs_entity readout,
	voe_platform_window *window, const struct voe_dev_timing *timing,
	uint32_t *glyphs, voe_base_error *error);

// One block, and then the period starts again. `seconds` is how long the period
// really lasted rather than VOE_DEV_REPORT_SECONDS, because a frame straddles
// the end of one.
void voe_dev_report(struct voe_dev_timing *timing, double seconds,
		    voe_render_present present);
