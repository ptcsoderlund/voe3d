// The frame breakdown: what the graphics card spent on each pass of a frame,
// by the pass's own name, in milliseconds to two places, and the frame's total
// (ADR-0358), drawn as an anchored panel at the right of the interface's area.
// The Frame button on src/interface.h's panel toggles it.
//
//     voe_dev_breakdown breakdown = { 0 };
//     voe_dev_breakdown_toggle(&breakdown);               // the Frame button
//     voe_dev_breakdown_take(&breakdown, gpu, seconds);   // each frame
//     if (breakdown.shown)
//             voe_dev_breakdown_draw(ui, &breakdown);      // inside the frame
//
// IT LAGS, AND THAT IS RENDER'S. The numbers are read after the frame slot's
// fence, so they describe the frame VOE_RENDER_FRAMES_IN_FLIGHT back
// (render/device.h, timing; ADR-0367 point 2); asking sooner would mean waiting
// for the card, which changes what is timed.
//
// FOUR TIMES A SECOND. A take copies the newest measurement at most every
// quarter second, because a number replaced every frame flickers too fast to
// read. Hidden, a take does nothing; a frame with no measurement keeps what was
// last shown, and "No GPU timings" stands until one ever is.
//
// Constraints. At most VOE_DEV_BREAKDOWN_PASSES passes; a frame with more shows
// its first ones, and raising the number with the budget below lifts it. Its
// cost in src/interface.h's budget: NODES, AT MOST 197 — the panel, one; the
// title, one; per pass a row and two labels, 192; the total's row and two
// labels, three. ELEMENTS, AT MOST 2971 — the panel's border and fill, two;
// "Frame", five; per pass its name, at most VOE_RENDER_PASS_NAME - 1 (31)
// letters, and its milliseconds, at most VOE_DEV_BREAKDOWN_MILLISECONDS - 1
// (15), 2944; "Total" and its milliseconds, twenty.
#pragma once

#include <render/device.h>
#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// The most passes one showing lists.
#define VOE_DEV_BREAKDOWN_PASSES 64
// The bytes of one milliseconds label with its NUL.
#define VOE_DEV_BREAKDOWN_MILLISECONDS 16

// Zeroed is a breakdown never shown and never measured.
typedef struct {
	bool shown;
	// Whether any measurement was ever taken; until then the panel says so.
	bool measured;
	uint32_t count;
	voe_render_pass_time passes[VOE_DEV_BREAKDOWN_PASSES];
	double total;
	// Kept here because a label's text is drawn after the call that made
	// it (ui/widgets.h); the names are drawn from `passes`.
	char milliseconds[VOE_DEV_BREAKDOWN_PASSES]
			 [VOE_DEV_BREAKDOWN_MILLISECONDS];
	char total_milliseconds[VOE_DEV_BREAKDOWN_MILLISECONDS];
	float since_refresh;
} voe_dev_breakdown;

// Shows a hidden breakdown, so the next take copies at once, or hides a shown one.
void voe_dev_breakdown_toggle(voe_dev_breakdown *breakdown);

// While shown, adds `seconds` to the time since the last refresh and, once a
// quarter second has passed, copies `gpu`'s newest pass times and frame time in.
void voe_dev_breakdown_take(voe_dev_breakdown *breakdown,
			    const voe_render_device *gpu, float seconds);

// The panel, anchored to the top right of the container it is drawn in.
// Asserts when it is not shown.
void voe_dev_breakdown_draw(voe_ui_context *ui,
			    const voe_dev_breakdown *breakdown);
