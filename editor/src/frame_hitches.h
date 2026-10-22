// The editor's hitch watch: every frame's CPU stretches timed, and one line on
// stderr, after `editor: hitch: `, for each frame that stalls on the CPU or on
// the card, naming what took the time (decisions 0458 point 3, 0461).
//
//     voe_editor_frame_hitches hitches = { 0 };
//     voe_editor_frame_hitches_begin(&hitches);         // top of the loop body
//     ... voe_editor_frame_hitches_mark(&hitches, "open"); ...
//     voe_editor_frame_hitches_end(&hitches, gpu, opened.waited);
//
// WHY. Bugs 08 and 11: the editor stalls in lumberhill, and which work stalls
// is not known. The Frame panel copies one frame four times a second, so a
// single stalled frame slips past it; a line the stalled frame writes itself
// cannot. The rule and the line's shape are app/hitches.h's.
//
// THE FRAME IS THE WATCH'S OWN SPAN, _begin's clock reading to _end's (0463
// point 1), so every stretch lies inside it. Bug 12: the tick's span put
// `pace` past its frame and left 13 s unnamed. What runs after _end and
// before the next _begin is in no frame.
//
// THE STRETCHES, as main.c marks them, each from the last reading to its mark
// (0461 point 1): `open` the top of the loop to the world step (loads, the
// theme, a built refresh, saves, history); `world` the world step; `undo` the
// undo settle; `pace` voe_app_frame_open; `step` the input, commands, pointer,
// view fits and the models' and Assets' updates; `fence` the draw's open, the
// wait for the slot's fence; `views` the landscapes' heights and the preview
// and view passes; `interface` the window pass and the interface's draw; `end`
// the frame closed, submitted and presented. Within `pace`, `pace: wait` is
// 0215's quarter second wait while unfocused, nought when focused; the rest of
// `pace` is the clock read and the poll (0463 point 2).
//
// WITHIN `views`, listed beside it (0461 points 2 and 3): `shadows` the
// seconds of the views' voe_3d_draw_system_shadows calls, and within that its
// parts, each summed over the views, `shadows: casters`, `shadows: ground`,
// `shadows: foliage`, `shadows: lamps` and `shadows: bounce`; `gather` the
// views' voe_3d_draw_system_foliage calls. Within `shadows: bounce` (0463
// point 3): `bounce: fit` the still casters' box and grid fit, `bounce: stale`
// the stale and removed spheres' walks, `bounce: begin` render's bounce
// begins, `bounce: captures` the capture passes, `bounce: suns` the bounce sun
// maps with foliage, `bounce: relight` render's relights. A part at 0 is left
// out. view_passes.c adds to them. The frame line ends `, light bounces` when
// any shadows call ran the bounce, else `, no light bounces` (0463 point 4).
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

// The most stretches one frame holds: nine marks, `pace: wait` and thirteen
// of the shadows', with one to spare (0463 point 4).
#define VOE_EDITOR_FRAME_HITCHES_STRETCHES 24

// Zeroed is ready. This file's; read it, change it only through the calls,
// but `shadows`, `gather` and `parts`, which the view passes add to.
typedef struct {
	voe_app_hitches hitches;
	// _begin's clock reading, where the frame begins.
	double began;
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

// Adds the stretches `shadows`, its parts and `gather` (within `views`) and
// `waited`, voe_app_frame.waited, as `pace: wait`; reads the clock and folds
// the seconds since _begin into the frame mean and, when the card has a
// measured total, that total and its passes into the card's; each hitch
// writes its line to stderr.
void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
				  const voe_render_device *gpu, double waited);
