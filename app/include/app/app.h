// The parts every program's frame loop repeats: a window and a device opened
// together, a frame opened, a draw opened and closed. A program calls them from
// a `while` it writes itself.
//
//     voe_app_settings settings = {
//             .width = 960, .height = 540, .title = "voe3d",
//             .capacities = { .vertices = 1 << 16, .indices = 1 << 17,
//                             .geometries = 64, .objects = 256, .shadings = 64 },
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
//             if (!voe_app_draw_open(app, frame.size, view, sun, &drawing))
//                     break;
//             if (drawing) {
//                     ... draws ...
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
// will not open and a GPU that stops answering are the world's doing and come
// back as NULL or false, for the program to decide about. A NULL app, a NULL
// title or a `longest_step` at or below zero is the program's own bug and
// asserts.
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

// Closes the device, then the window. The struct itself is the arena's and is
// not freed here.
void voe_app_destroy(voe_app *app);

// The window and the device, for everything this folder deliberately does not
// wrap — input, a texture uploaded at startup, a present mode. A program reaches
// for these often and that is not a gap: see the list of what is not here.
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

// Opens the recording with `view` as the camera and `light` as the sun. False
// means the device cannot draw any more and the program should stop asking; a
// line saying so is on stderr.
//
// `drawing` COMES BACK FALSE WHEN THERE IS NOTHING TO DRAW INTO and that is not
// a failure — issue no draws, do not call _draw_close, and go round again.
[[nodiscard]] bool voe_app_draw_open(voe_app *app, voe_platform_size size,
				     voe_render_view view, voe_render_light light,
				     bool *drawing);

// Submits the frame and presents it. Called only when _draw_open set `drawing`;
// false on the same terms and with the same line.
[[nodiscard]] bool voe_app_draw_close(voe_app *app);
