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
//             if (!voe_render_device_frame(gpu, voe_platform_window_size(window)))
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
// WHAT IT DRAWS IS ONE CUBE AND THERE IS NO WAY TO ASK FOR ANYTHING ELSE. The
// geometry, the camera and the matrices are all constants inside the folder, the
// shader is compiled into the binary and the draw is fixed; the point of it is
// that the buffers, the depth test and the matrix upload work end to end, not
// that the renderer takes instructions. Nothing here accepts a scene, and the
// API that will grow one gets designed against 3d.
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

// Draws the frame and puts it on the window. size is the window's current client
// area; when it differs from the one the frame was last built for, the offscreen
// targets and the swapchain are both rebuilt here before anything is drawn.
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
					   voe_platform_size size);
