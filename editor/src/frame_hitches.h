// The editor's hitch watch: every frame's CPU stretches timed, and one line on
// stderr, after `editor: hitch: `, for each frame that stalls on the CPU or on
// the card, naming what took the time (decision 0458 point 3).
//
//     voe_editor_frame_hitches hitches = { 0 };
//     voe_editor_frame_hitches_begin(&hitches);         // top of the loop body
//     ... voe_editor_frame_hitches_mark(&hitches, "open"); ...
//     voe_editor_frame_hitches_end(&hitches, gpu, tick.elapsed, shadows);
//
// WHY. Bug 08: the editor stutters in lumberhill while the sun bounces, and
// which of the bounce's works stalls is not known. The Frame panel copies one
// frame four times a second, so a single stalled frame slips past it; a line
// the stalled frame writes itself cannot. The rule and the line's shape are
// app/hitches.h's.
//
// THE STRETCHES, as main.c marks them, each from the last reading to its mark:
// `open` the top of the loop to the frame's open (loads, a built refresh, the
// theme, saves, history, world step, undo settle, clock and poll); `step` the
// input, commands, pointer, view fits and the models' and Assets' updates;
// `views` the draw's open (the wait for the slot's fence), the landscapes'
// heights and the preview and view passes; `interface` the window pass and
// the interface's draw; `end` the frame closed, submitted and presented.
// `shadows` is the seconds the views' voe_3d_draw_system_shadows calls took,
// part of `views` and listed beside it, as the bounce runs in them.
//
// THE CARD'S LINE LAGS. Its total and passes are render's newest measurement,
// VOE_RENDER_FRAMES_IN_FLIGHT frames back, as the Frame panel's are; a frame
// line and its card line may be two frames apart.
//
// Constraints: always on, a clock read per mark and nothing written until a
// frame stalls. At most VOE_EDITOR_FRAME_HITCHES_STRETCHES stretches a frame,
// the shadows included; a mark past them asserts. At most
// VOE_EDITOR_FRAME_PASSES passes on the card's line, its first ones.
#pragma once

#include <app/hitches.h>

#include <render/device.h>

#include <stdint.h>

// The most stretches one frame holds, the shadows included.
#define VOE_EDITOR_FRAME_HITCHES_STRETCHES 8

// Zeroed is ready. This file's; read it, change it only through the calls.
typedef struct {
	voe_app_hitches hitches;
	// The platform clock's last reading, where the next stretch begins.
	double since;
	uint32_t count;
	voe_app_stretch stretches[VOE_EDITOR_FRAME_HITCHES_STRETCHES];
} voe_editor_frame_hitches;

// At the top of a frame: drops last frame's stretches and reads the clock.
void voe_editor_frame_hitches_begin(voe_editor_frame_hitches *watch);

// Ends the stretch since the last reading, named `name` (a string that
// outlives the frame), and reads the clock again.
void voe_editor_frame_hitches_mark(voe_editor_frame_hitches *watch,
				   const char *name);

// Adds the stretch `shadows` (within `views`), folds `elapsed` into the frame
// mean and, when the card has a measured total, that total and its passes into
// the card's; each hitch writes its line to stderr.
void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
				  const voe_render_device *gpu, double elapsed,
				  double shadows);
