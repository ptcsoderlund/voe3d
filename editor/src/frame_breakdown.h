// The Frame panel: what the graphics card spent on each pass of a frame, by the
// pass's own name, in milliseconds, and the frame's total (ADR-0358), so render
// work is argued from a measurement. A title row "Frame" with an × at its right
// (0363 point 2), a row per pass, name left and milliseconds right, then Total —
// an anchored panel at a point the caller gives.
//
//     voe_editor_frame_breakdown breakdown = { 0 };
//     voe_editor_frame_breakdown_show(&breakdown);
//     voe_editor_frame_breakdown_take(&breakdown, gpu, seconds); // each frame
//     if (breakdown.showing)
//             voe_editor_frame_breakdown_draw(ui, &breakdown, at);
//     ... voe_ui_frame_end ...
//     if (voe_editor_frame_breakdown_clicks_read(ui, &breakdown))
//             voe_editor_frame_breakdown_hide(&breakdown);
//
// IT LAGS, AND THAT IS RENDER'S, NOT THIS FILE'S. The numbers are read from the
// card after the frame slot's fence, so they describe the frame
// VOE_RENDER_FRAMES_IN_FLIGHT back (render/device.h, timing; ADR-0367 point 2);
// asking sooner would mean waiting for the card, which changes what is timed.
//
// FOUR TIMES A SECOND. The take copies the newest measurement at most every
// quarter second: a number replaced every frame flickers too fast to read, and
// a quarter second is still quick enough to see a change follow an edit. A
// frame with no measurement, or a card that writes no timestamps, keeps what
// was last shown; "No GPU timings" stands until one ever is.
//
// Constraints. At most VOE_EDITOR_FRAME_PASSES passes; a frame with more shows
// its first ones, and raising the number, with interface.h's budget, lifts it.
// NODES, AT MOST 200: the panel, one; the title row, its label and the × as a
// button and a label, four; per pass a row and two labels, three, 192; the
// total's row and two labels, three. ELEMENTS, AT MOST 2974: the panel's border
// and fill, two; "Frame", five; the ×'s border, fill and character, three; per
// pass its name, at most VOE_RENDER_PASS_NAME - 1 (31) characters, and its
// milliseconds, at most VOE_EDITOR_FRAME_MILLISECONDS - 1 (15), 2944; "Total"
// and its milliseconds, twenty. Those are what interface.h's budget counts.
#pragma once

#include <math/float2.h>

#include <render/device.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// The most passes one showing lists.
#define VOE_EDITOR_FRAME_PASSES 64
// The bytes of one milliseconds label with its NUL.
#define VOE_EDITOR_FRAME_MILLISECONDS 16

// Zeroed is a panel never shown and never measured.
typedef struct {
	bool showing;
	// Whether any measurement was ever taken; until then the panel says so.
	bool measured;
	uint32_t count;
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	double total;
	// Kept here because a label's text is drawn after the call that made
	// it (ui/widgets.h); the names are drawn from `passes`.
	char milliseconds[VOE_EDITOR_FRAME_PASSES][VOE_EDITOR_FRAME_MILLISECONDS];
	char total_milliseconds[VOE_EDITOR_FRAME_MILLISECONDS];
	float since_refresh;
	voe_ui_node close_button;
} voe_editor_frame_breakdown;

void voe_editor_frame_breakdown_show(voe_editor_frame_breakdown *breakdown);
void voe_editor_frame_breakdown_hide(voe_editor_frame_breakdown *breakdown);

// While showing, adds `seconds` to the time since the last refresh and, once a
// quarter second has passed, copies `gpu`'s newest pass times and frame time in.
// Not showing, or no measurement, it changes nothing shown.
void voe_editor_frame_breakdown_take(voe_editor_frame_breakdown *breakdown,
				     const voe_render_device *gpu, float seconds);

// Draws the panel with its top left at `at` and records its ×. Asserts when it
// is not showing.
void voe_editor_frame_breakdown_draw(voe_ui_context *ui,
				     voe_editor_frame_breakdown *breakdown,
				     voe_math_float2 at);

// Whether the × fired this frame. Called after voe_ui_frame_end and before
// the frame's arena is rewound — the one window in which a widget answers.
bool voe_editor_frame_breakdown_clicks_read(
	const voe_ui_context *ui, const voe_editor_frame_breakdown *breakdown);
