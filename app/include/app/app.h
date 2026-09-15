// The parts every program's frame loop repeats: a window and a device opened
// together — or a device alone, with no window at all — a frame opened, a draw
// opened and closed, and what was drawn written out as a picture. A program
// calls them from a `while` it writes itself.
//
//     voe_app_settings settings = {
//             .width = 960, .height = 540, .title = "voe3d",
//             .capacities = { .vertices = 1 << 16, .indices = 1 << 17,
//                             .geometries = 64, .objects = 256, .shadings = 64,
//                             .passes = 1 },
//             .longest_step = 0.25,
//     };
//     voe_app *app = voe_app_new(arena, scratch, settings, &error);
//     if (app == NULL)
//             return 1;
//
//     while (true) {
//             voe_app_frame frame = voe_app_frame_open(app);
//             if (frame.closing)
//                     break;
//             world_advance(frame.tick.step);
//             if (frame.minimised)
//                     continue;
//
//             bool drawing;
//             if (!voe_app_draw_open(app, frame.size, &drawing))
//                     break;
//             if (drawing) {
//                     voe_render_pass_camera camera = { view, sun };
//                     if (voe_render_pass_begin(voe_app_device(app),
//                                               VOE_RENDER_TARGET_WINDOW, &camera)) {
//                             ... draws ...
//                             voe_render_pass_end(voe_app_device(app));
//                     }
//                     if (!voe_app_draw_close(app))
//                             break;
//             }
//     }
//     voe_app_destroy(app);
//
// THERE IS NO LOOP IN HERE, NO CALLBACK AND NO FUNCTION POINTER, AND THAT IS
// THE POINT OF THE FOLDER (ADR-0135). The dozen lines above are the program's
// own and stay readable in one screen; what is factored out is the part that is
// the same in every program and easy to get subtly wrong — the order of the
// poll against the tick, the draw that must not be closed when it never opened.
// A folder that owned the loop would have to be told, through some callback,
// everything a program wants to do inside one, and every program would then be
// shaped by this file instead of by itself.
//
// THERE ARE TWO STARTUPS AND ONE LOOP (ADR-0157). voe_app_new opens a window and
// a device onto it; voe_app_new_headless opens the device alone, at the size the
// settings name, and every line of the loop above is then unchanged — which is
// the point, because a frame drawn with no display has to be the frame a person
// would have seen rather than the output of a second path kept in step by hand.
// What differs is only what there is to ask: voe_app_window is NULL, so a
// program that may be started either way guards its input reads with it and
// reads zeroed input when there is none, and voe_app_frame_open polls nothing,
// reports the settings' size every frame, and is never `closing` and never
// `minimised` — so a headless loop has to stop itself, because nothing will ever
// tell it to. The clock ticks either way.
//
// AND WHAT WAS DRAWN COMES BACK OUT AS A FILE. voe_app_capture_png is the one
// call for it, over render's readback, assets' encoder and platform's file
// write; it is called between frames and never inside one. It is the only thing
// in this folder that is not part of a frame, and it is here because it is the
// three folders a program would otherwise have to name itself.
//
// SO A LONG LIST OF THINGS IS NOT HERE AND IS NOT COMING. The order systems run
// in, a world and what is registered in it, any arena beyond the two handed to
// _new, the timing readout, what a key is bound to, and which present mode the
// device opens on — all the program's. _new requests no present mode at all:
// the device opens on FIFO and a program that wants otherwise asks render
// itself (ADR-0131).
//
// THE TWO ARENAS ARE DIFFERENT AND BOTH ARE THE CALLER'S. `arena` holds this
// struct and has to outlive it. `scratch` is startup's working memory, handed
// straight to render, and nothing is kept out of it — rewind or destroy it as
// soon as _new returns.
//
// WHAT IS RETURNED AND WHAT ASSERTS follows the engine's rule: a window that
// will not open, a GPU that stops answering and a file that will not be written
// are the world's doing and come back as NULL or false, for the program to
// decide about. A NULL app or a `longest_step` at or below zero is the program's
// own bug and asserts, and so is each startup's own missing piece — a NULL title
// to voe_app_new, which has a window to put one on, and a width or height at or
// below zero to voe_app_new_headless, where the size is the whole of what gets
// drawn. voe_app_new_headless needs no title and takes a NULL one.
#pragma once

#include <app/clock.h>

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>
#include <render/device.h>

typedef struct voe_app voe_app;

// `capacities` is handed to render untouched — see render/include/render/device.h
// for what each number buys. `longest_step` is the ceiling on a frame's step, in
// seconds, and is the same number every tick; see app/clock.h for why it exists.
typedef struct {
	int width;
	int height;
	const char *title;
	voe_render_capacities capacities;
	double longest_step;
} voe_app_settings;

// Opens the window, then the device onto it. NULL if either refused, with a line
// on stderr saying which; `error` may be NULL.
//
// A WINDOW THAT WILL NOT OPEN IS VOE_BASE_ERROR_UNAVAILABLE, because that is the
// only way it fails: there is no compositor, no display, or no session. A device
// that will not start passes render's own error through, which tells three cases
// apart, and the window is closed again before this returns.
[[nodiscard]] voe_app *voe_app_new(voe_base_arena *arena, voe_base_arena *scratch,
				   voe_app_settings settings,
				   voe_base_error *error);

// The same app with no window at all: the headless device alone, drawing at
// `settings.width` by `settings.height`. `settings.title` is unused and may be
// NULL here, because there is nothing to put a title on. NULL if the device
// refused, with render's own error and its own line on stderr.
//
// ONE DRAWING PATH, WITH OR WITHOUT A DISPLAY, AND THAT IS THE WHOLE POINT
// (ADR-0157). Everything after this call is what a windowed program already
// writes — the same frame open, the same passes, the same draws, the same
// _draw_close — so a frame captured with no display is the frame a person would
// have seen rather than the output of a second path kept in step by hand.
//
// voe_app_window RETURNS NULL FOR SUCH AN APP AND THAT IS THE HONEST ANSWER. A
// program that wants the pointer, the wheel or the keyboard is asking a window
// it did not ask for; there is nothing to hand back and nothing sensible to
// invent. A program meant to run both ways guards its input reads with
// `voe_app_window(app) != NULL` and reads zeroed input when there is none.
//
// voe_app_frame_open DOES NOT POLL — there is no window to poll and platform has
// nothing else to ask. It reports `settings.width` by `settings.height` as the
// size every frame, is never `closing` and never `minimised`, and still ticks
// the clock exactly as it does with a window, so a loop that advances a world by
// `frame.tick.step` behaves the same. A headless loop therefore has to stop
// itself: nothing will ever tell it to.
//
// A width or height at or below zero asserts. With a window the size is the
// window system's answer and may be anything; here it is the caller's own
// number and the whole of what gets drawn.
[[nodiscard]] voe_app *voe_app_new_headless(voe_base_arena *arena,
					    voe_base_arena *scratch,
					    voe_app_settings settings,
					    voe_base_error *error);

// Closes the device, then the window if there is one — an app opened headless
// has none to close. The struct itself is the arena's and is not freed here.
void voe_app_destroy(voe_app *app);

// The window and the device, for everything this folder deliberately does not
// wrap — input, a texture uploaded at startup, a present mode. A program reaches
// for these often and that is not a gap: see the list of what is not here.
//
// THE WINDOW IS NULL ON AN APP OPENED BY voe_app_new_headless, and a program
// that may be started either way checks for it rather than assuming. The device
// is never NULL.
voe_platform_window *voe_app_window(voe_app *app);
voe_render_device *voe_app_device(voe_app *app);

// `size` is the client area this frame, which is what a draw is opened with.
// `minimised` is that size with no area — nothing can be drawn into it, and the
// program should skip the draw and keep looping rather than stop.
typedef struct {
	voe_app_tick tick;
	voe_platform_size size;
	bool minimised;
	bool closing;
} voe_app_frame;

// Reads the clock, ticks it, polls the window, and reports what the window then
// says. Once at the top of the loop.
//
// THE POLL IS IN HERE AND IT IS THE ONLY ONE THERE IS. platform has no second
// poll and no event queue, so a loop that does not call this gets a window that
// never closes and a keyboard that never moves. It does not wait, so a loop with
// nothing else in it spins a core.
voe_app_frame voe_app_frame_open(voe_app *app);

// Opens the recording. It draws nothing and has no camera: every draw is inside a
// pass the program opens on the device — see render/device.h. False means the
// device cannot draw any more and the program should stop asking; a line saying
// so is on stderr.
//
// `drawing` COMES BACK FALSE WHEN THERE IS NOTHING TO DRAW INTO and that is not
// a failure — issue no draws, do not call _draw_close, and go round again.
[[nodiscard]] bool voe_app_draw_open(voe_app *app, voe_platform_size size,
				     bool *drawing);

// Submits the frame and presents it. Called only when _draw_open set `drawing`;
// false on the same terms and with the same line.
[[nodiscard]] bool voe_app_draw_close(voe_app *app);

// Reads `target` back, encodes it as a PNG and writes the file at `path`. Three
// folders' work — render's readback, assets' encoder, platform's file — in the
// one call a program that wants a picture of what it drew actually means
// (ADR-0157).
//
// BETWEEN FRAMES AND NEVER INSIDE ONE. The readback waits for the card to go
// idle, so this belongs after voe_app_draw_close and not between a pass's begin
// and end; calling it with a frame open asserts, in render, where the rule is.
// It is not a per-frame call for the same reason.
//
// `scratch` HOLDS THE PICTURE AND THE FILE'S BYTES AND NOTHING SURVIVES IT. The
// pixels off the card, the working memory the encoder needs and the encoded file
// all come out of it, and none of them is looked at again once the file is
// written — so it may be rewound or destroyed the moment this returns, and a
// program capturing repeatedly rewinds rather than growing.
//
// VOE_RENDER_TARGET_WINDOW IS WHAT A PROGRAM MEANS BY "WHAT I JUST DREW", on a
// windowed device and on a headless one alike. A target of its own is what it
// means by one particular view.
//
// FALSE COMES STRAIGHT FROM WHICHEVER OF THE THREE STEPS REFUSED, with that
// folder's category in `error` and its own line already on stderr; nothing is
// added here, because "the disk is full" and "the card refused a buffer" are
// already as specific as they are going to get. NOTHING IS WRITTEN WHEN AN
// EARLIER STEP FAILED — the file is opened by the last step and only if the
// first two produced bytes. `error` may be NULL.
[[nodiscard]] bool voe_app_capture_png(voe_app *app, voe_render_target target,
				       voe_base_arena *scratch,
				       const char *path,
				       voe_base_error *error);
