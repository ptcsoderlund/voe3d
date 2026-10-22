// The editor's hitch watch: every frame's CPU stretches timed, and one line on
// stderr, after `editor: hitch: `, for each frame that stalls on the CPU or on
// the card, naming what took the time (decisions 0458 point 3, 0461).
//
//     voe_editor_frame_hitches hitches = { 0 };
//     voe_editor_frame_hitches_begin(&hitches);         // top of the loop body
//     ... voe_editor_frame_hitches_mark(&hitches, "open"); ...
//     voe_editor_frame_hitches_end(&hitches, gpu, tick.elapsed);
//
// WHY. Bugs 08 and 11: the editor stalls in lumberhill, and which work stalls
// is not known. The Frame panel copies one frame four times a second, so a
// single stalled frame slips past it; a line the stalled frame writes itself
// cannot. The rule and the line's shape are app/hitches.h's.
//
// THE STRETCHES, as main.c marks them, each from the last reading to its mark
// (0461 point 1): `open` the top of the loop to the world step (loads, the
// theme, a built refresh, saves, history); `world` the world step; `undo` the
// undo settle; `pace` voe_app_frame_open, its clock, poll and 0215's quarter
// second wait while unfocused, nought when focused; `step` the input,
// commands, pointer, view fits and the models' and Assets' updates; `fence`
// the draw's open, the wait for the slot's fence; `views` the landscapes'
// heights and the preview and view passes; `interface` the window pass and the
// interface's draw; `end` the frame closed, submitted and presented.
//
// WITHIN `views`, listed beside it (0461 points 2 and 3): `shadows` the
// seconds of the views' voe_3d_draw_system_shadows calls, and within that its
// parts, each summed over the views, `shadows: casters`, `shadows: ground`,
// `shadows: foliage`, `shadows: lamps` and `shadows: bounce`, one at 0 left
// out; `gather` the views' voe_3d_draw_system_foliage calls. view_passes.c
// adds to them.
//
// THE CARD'S LINE LAGS. Its total and passes are render's newest measurement,
// VOE_RENDER_FRAMES_IN_FLIGHT frames back, as the Frame panel's are; a frame
// line and its card line may be two frames apart. It ends with `, <n>
// triangles, <m> draws`, render's counts of the newest recorded frame, this
// one, not the measured one (0461 point 4).
//
// Constraints: always on, a clock read per mark and nothing written until a
// frame stalls. At most VOE_EDITOR_FRAME_HITCHES_STRETCHES stretches a frame,
// the shadows' included; one past them asserts. At most
// VOE_EDITOR_FRAME_PASSES passes on the card's line, its first ones.
#pragma once

#include <3d/draw_system.h>

#include <app/hitches.h>

#include <render/device.h>

#include <stdint.h>

// The most stretches one frame holds: nine marks and seven of the shadows'.
#define VOE_EDITOR_FRAME_HITCHES_STRETCHES 16

// Zeroed is ready. This file's; read it, change it only through the calls,
// but `shadows`, `gather` and `parts`, which the view passes add to.
typedef struct {
	voe_app_hitches hitches;
	// The platform clock's last reading, where the next stretch begins.
	double since;
	// This frame's seconds in the shadows calls, in the foliage gathers and
	// in each part of the shadows; zeroed by _begin.
	double shadows;
	double gather;
	voe_3d_shadow_times parts;
	uint32_t count;
	voe_app_stretch stretches[VOE_EDITOR_FRAME_HITCHES_STRETCHES];
} voe_editor_frame_hitches;

// At the top of a frame: drops last frame's stretches and sums, and reads the
// clock.
void voe_editor_frame_hitches_begin(voe_editor_frame_hitches *watch);

// Ends the stretch since the last reading, named `name` (a string that
// outlives the frame), and reads the clock again.
void voe_editor_frame_hitches_mark(voe_editor_frame_hitches *watch,
				   const char *name);

// Adds the stretches `shadows`, its parts and `gather` (within `views`), folds
// `elapsed` into the frame mean and, when the card has a measured total, that
// total and its passes into the card's; each hitch writes its line to stderr.
void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
				  const voe_render_device *gpu, double elapsed);
