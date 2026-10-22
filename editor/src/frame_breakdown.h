// The Frame panel: what the graphics card spent on each pass of a frame, by the
// pass's own name, in milliseconds, and the frame's total (ADR-0358), so render
// work is argued from a measurement. A title row "Frame" with an × at its right
// (0363 point 2), a row per pass, name left and milliseconds right, then Total,
// then Triangles, the frame's triangle count (ADR-0425 point 7), then Draws, its
// draw commands, far fewer than trees (0442), then two rows per picture drawn,
// `Camera` or `View N` with its things drawn and skipped and `  shadows` with its
// cascades' (0465 point 1) — an anchored panel at a point the caller gives.
//
//     voe_editor_frame_breakdown breakdown = { 0 };
//     voe_editor_frame_breakdown_show(&breakdown);
//     voe_editor_frame_breakdown_take(&breakdown, gpu, &views, seconds); // each frame
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
// refresh with no measurement, or a card that writes no timestamps, keeps the
// times last shown; "No GPU timings" stands until one ever is. The pictures'
// counts are the CPU's own, exact the frame they are made, so every refresh
// copies them, measured or not.
//
// Constraints. At most VOE_EDITOR_FRAME_PASSES passes; a frame with more shows
// its first ones, and raising the number, with interface.h's budget, lifts it.
// NODES, AT MOST 236: the panel, one; the title row, its label and the × as a
// button and a label, four; per pass a row and two labels, three, 192; the
// total's, the triangles' and the draws' rows and two labels each, nine; per
// picture, VOE_EDITOR_VIEWS + 1 (5), two rows and two labels each, thirty.
// ELEMENTS, AT MOST 3443: the panel's border and fill, two; "Frame", five; the
// ×'s border, fill and character, three; per pass its name, at most
// VOE_RENDER_PASS_NAME - 1 (31) characters, and its milliseconds, at most
// VOE_EDITOR_FRAME_MILLISECONDS - 1 (15), 2944; "Total" and its milliseconds,
// twenty; "Triangles" and at most VOE_EDITOR_FRAME_TRIANGLES - 1 (20) digits,
// twenty-nine; "Draws" and at most VOE_EDITOR_FRAME_DRAWS - 1 (10) digits,
// fifteen; per picture its name, at most "Camera"'s six, "  shadows", nine,
// and two counts of at most VOE_EDITOR_FRAME_COUNTS - 1 (35), eighty-five
// apiece, 425. Those are what interface.h's budget counts.
#pragma once

#include "view.h"

#include <math/float2.h>

#include <render/device.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// The most passes one showing lists.
#define VOE_EDITOR_FRAME_PASSES 64
// The bytes of one milliseconds label with its NUL.
#define VOE_EDITOR_FRAME_MILLISECONDS 16
// The bytes of the triangles label with its NUL: a uint64_t's 20 digits.
#define VOE_EDITOR_FRAME_TRIANGLES 21
// The bytes of the draws label with its NUL: a uint32_t's 10 digits.
#define VOE_EDITOR_FRAME_DRAWS 11
// The bytes of a picture's name with its NUL: "View " and a uint32_t's digits.
#define VOE_EDITOR_FRAME_PICTURE 16
// The bytes of one counts label with its NUL: "<drawn> drawn, <skipped> skipped"
// with two uint32_t's of 10 digits.
#define VOE_EDITOR_FRAME_COUNTS 36

// Zeroed is a panel never shown and never measured.
typedef struct {
	bool showing;
	// Whether any measurement was ever taken; until then the panel says so.
	bool measured;
	uint32_t count;
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	double total;
	uint64_t triangles;
	uint32_t draws;
	// The pictures drawn at the last refresh, in drawing order (view.h).
	voe_editor_picture_counts pictures[VOE_EDITOR_VIEWS + 1];
	uint32_t picture_count;
	// Kept here because a label's text is drawn after the call that made
	// it (ui/widgets.h); the names are drawn from `passes`.
	char milliseconds[VOE_EDITOR_FRAME_PASSES][VOE_EDITOR_FRAME_MILLISECONDS];
	char total_milliseconds[VOE_EDITOR_FRAME_MILLISECONDS];
	char triangles_label[VOE_EDITOR_FRAME_TRIANGLES];
	char draws_label[VOE_EDITOR_FRAME_DRAWS];
	char picture_names[VOE_EDITOR_VIEWS + 1][VOE_EDITOR_FRAME_PICTURE];
	char drawn_labels[VOE_EDITOR_VIEWS + 1][VOE_EDITOR_FRAME_COUNTS];
	char cast_labels[VOE_EDITOR_VIEWS + 1][VOE_EDITOR_FRAME_COUNTS];
	float since_refresh;
	voe_ui_node close_button;
} voe_editor_frame_breakdown;

void voe_editor_frame_breakdown_show(voe_editor_frame_breakdown *breakdown);
void voe_editor_frame_breakdown_hide(voe_editor_frame_breakdown *breakdown);

// While showing, adds `seconds` to the time since the last refresh and, once a
// quarter second has passed, copies `views`' pictures' counts in, then `gpu`'s
// newest pass times and frame time. The frame's triangle and draw counts are
// taken with the times and lag as they do. Not showing, it changes nothing
// shown; no measurement keeps the times shown.
void voe_editor_frame_breakdown_take(voe_editor_frame_breakdown *breakdown,
				     const voe_render_device *gpu,
				     const voe_editor_views *views,
				     float seconds);

// Draws the panel with its top left at `at` and records its ×. Asserts when it
// is not showing.
void voe_editor_frame_breakdown_draw(voe_ui_context *ui,
				     voe_editor_frame_breakdown *breakdown,
				     voe_math_float2 at);

// Whether the × fired this frame. Called after voe_ui_frame_end and before
// the frame's arena is rewound — the one window in which a widget answers.
bool voe_editor_frame_breakdown_clicks_read(
	const voe_ui_context *ui, const voe_editor_frame_breakdown *breakdown);
