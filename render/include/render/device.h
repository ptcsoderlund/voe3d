// The GPU, opened onto a window. Create one, ask it for a frame every time round
// the loop, destroy it. That is the whole public surface of render today, and it
// is deliberately close to nothing: the shape render should really have gets
// designed against 3d, which does not exist yet, and inventing it now would be
// inventing it blind.
//
//     voe_base_arena *scratch = voe_base_arena_new(64 * 1024);
//     voe_base_error error;
//     voe_render_device *gpu = voe_render_device_new(scratch,
//                                                    voe_platform_window_native(window),
//                                                    voe_platform_window_size(window),
//                                                    &error);
//     voe_base_arena_destroy(scratch);        // the device kept nothing from it
//     if (gpu == NULL) {
//             fprintf(stderr, "%s\n", voe_base_error_string(error));
//             return 1;
//     }
//     while (!voe_platform_window_should_close(window)) {
//             voe_platform_window_poll(window);
//             voe_render_camera_input look = { 0 };       // the orbit
//             if (!voe_render_device_frame(gpu, voe_platform_window_size(window),
//                                          look))
//                     break;
//     }
//     voe_render_device_destroy(gpu);
//
// ONE OBJECT, AND IT IS FOUR VULKAN OBJECTS IN A TRENCH COAT — the instance, the
// surface, the logical device and the swapchain. They have one lifetime and one
// owner today, and splitting them into four types with four _new functions would
// be four public names where nothing yet needs even one of them apart. When 3d
// arrives and something needs a device without a window, that is the card that
// splits this.
//
// IT DRAWS TO AN IMAGE OF ITS OWN AND COPIES THAT TO THE WINDOW. The frame is
// cleared and drawn into an offscreen colour target, and putting that on screen
// is a separate last step — which is what any post process, a render resolution
// the window is not, and an editor viewport all need in order to be possible at
// all. None of it is visible from out here: the same call draws the same frame.
//
// WHAT IT DRAWS IS TWO CUBES AND THERE IS NO WAY TO ASK FOR ANYTHING ELSE. The
// geometry, the two matrices and the motion of the turning cube are constants
// inside the folder, the shader is compiled into the binary and the draws are
// fixed; the point of it is that the buffers, the depth test, the matrix upload
// and a matrix per object work end to end, not that the renderer takes
// instructions. Nothing here accepts a scene, and the API that will grow one
// gets designed against 3d.
//
// THE CAMERA IS THE ONE THING A CALLER CAN MOVE, AND IT IS MOVED BY DESCRIBING
// WHAT THE PERSON DID. voe_render_camera_input below is what _frame takes: a
// direction to walk in, how far the mouse moved, and whether to fly at all. It
// is not a position and not a matrix, because the camera itself is still this
// folder's — its speed, its field of view and how far a mouse turns it are
// constants in cube.c beside the orbit they replaced. The card that moves the
// camera out of render is the card that gives it a folder of its own; until
// then, this is the seam.
//
// AND IT IS NOT A WINDOW. render never holds a platform window and never asks
// one a question, for the same reason the size below is passed in: the caller
// has already read the keyboard, and a renderer that read it again would be a
// second place that decides what the input means. Which key moves which way is
// the caller's; what a camera does about it is this folder's.
//
// THE ORBIT FROM CARD 015 IS WHAT A ZEROED voe_render_camera_input GIVES, AND
// THAT IS DELIBERATE RATHER THAN A DEFAULT THAT FELL OUT. Three motions with
// nobody touching anything is the thing to look at when what is being checked is
// the rendering; a camera under a hand is the thing to look at when what is
// being checked is the input. Both are wanted, so `fly` chooses, and the tests
// in this folder that draw a frame get the orbit without knowing the field
// exists.
//
// THE TURNING CUBE STILL MOVES ON ITS OWN AND THE CALLER IS NOT ASKED WHAT TIME
// IT IS. Every call to _frame advances the clock by a nominal frame's worth of
// time, so the spin's speed — and the orbit's, when it is the orbit — follows
// the display's refresh rate until something in the engine can measure a frame.
// Card 020 is that card.
//
// IT IS SOLID, WHICH MEANS THERE IS A DEPTH BUFFER BEHIND THIS CALL. One per
// frame slot, rebuilt on a resize with the colour target, and never visible from
// out here. This engine's depth runs backwards — near at 1.0, far at 0.0 — and
// nothing outside the folder has to know that either.
//
// SIZE IS PASSED IN, EVERY FRAME, AND IT IS THE WINDOW'S ANSWER. This folder
// never asks the window how big it is: platform owns that truth, the caller
// already has it, and the alternative is render holding a window pointer so it
// can ask a question it was told the answer to. A window that has changed size
// is not an event here — the size simply differs from the one the swapchain was
// built at, and the next frame rebuilds it.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

typedef struct voe_render_device voe_render_device;

// native and size come from voe_platform_window_native() and
// voe_platform_window_size(). arena is scratch for the enumerations startup
// does — how many graphics cards, which queue families, which surface formats —
// and nothing the device keeps is allocated out of it, so it may be rewound or
// destroyed as soon as this returns.
//
// NULL on failure, with error saying which category it was and a line on stderr
// saying exactly what happened. There are three ways this fails and a caller can
// tell them apart: no Vulkan on the machine, no graphics card that meets what
// the engine requires, and a driver that refused. error may be NULL if the
// caller does not care which.
[[nodiscard]] voe_render_device *voe_render_device_new(voe_base_arena *arena,
						       voe_platform_native native,
						       voe_platform_size size,
						       voe_base_error *error);
void voe_render_device_destroy(voe_render_device *device);

// What the person asked the camera to do since the last frame. A zeroed one asks
// for nothing, which is the orbit — see the header.
//
// THE THREE MOVEMENT FIELDS ARE A DIRECTION AND NOT A SPEED. Each is -1 to 1 in
// the camera's own frame: forward is along where it is looking, right is to its
// right, and up is world up rather than the camera's, so looking at the floor
// and pressing up still goes up. How fast that is, is cube.c's. A caller that
// puts 2.0 in one of these is asking to go twice as fast and will, which is not
// a promise — it is what falls out of multiplying, and `fast` is the field that
// means it.
//
// look_x AND look_y ARE THE MOUSE'S NUMBERS UNTOUCHED, AND THAT IS WHY THEY HAVE
// NO UNIT HERE. voe_platform_input_motion hands out a delta whose scale is the
// window system's, and turning that into an angle needs a sensitivity that
// belongs with the camera. So a caller passes what platform said and does no
// arithmetic on it; cube.c owns the constant. +x is right and +y is down, which
// is what platform reports.
typedef struct {
	float forward;
	float right;
	float up;
	float look_x;
	float look_y;
	// Move faster while it is held. A multiplier's worth, and the
	// multiplier is cube.c's.
	bool fast;
	// False is the orbit. The frame this turns true, the camera takes over
	// from wherever the orbit had got to, so nothing jumps.
	bool fly;
} voe_render_camera_input;

// Draws the frame and puts it on the window. size is the window's current client
// area; when it differs from the one the frame was last built for, the offscreen
// targets and the swapchain are both rebuilt here before anything is drawn. look
// is what the person did since the last call.
//
// A zero-sized window — minimised, or mid-resize on some compositors — draws
// nothing and returns true. It is not an error and it must not be treated as
// one; it also does not wait, so a loop that does nothing else will spin.
//
// False means this device cannot draw any more and the program should stop
// asking: the driver refused something no retry will fix. Everything a frame can
// hit that a retry does fix — a swapchain gone stale, a window resized under us —
// is handled here and returns true.
[[nodiscard]] bool voe_render_device_frame(voe_render_device *device,
					   voe_platform_size size,
					   voe_render_camera_input look);
