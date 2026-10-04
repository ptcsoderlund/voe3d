// The game's side of a project's interface (0259): the font, theme and ui
// context it is drawn with, and one frame of it begun, pointed at and handed
// to the project's entry point.
//
//     voe_game_interface *interface = voe_game_interface_new(device, arena);
//     ...each frame, after the steps:
//     voe_game_project_asks asks = { 0 };              // kept by the run
//     if (!voe_game_interface_run(interface, frame_arena, world, window, size,
//                                 &asks, voe_game_project_interface))
//             ...                                  // the project ended the run
//     ...read asks.paused and asks.restart
//     ...draw voe_game_interface_context(interface)'s element records
//     voe_game_interface_destroy(interface);
//
// THE SURFACE IS VOE_GAME_SURFACE_HIGH MILLIMETRES TALL, as wide as the
// window's aspect: one scale on both axes, pixels per millimetre the window's
// height over 135 (the editor's ADR-0104 rule). The pointer's pixels are
// divided by that same number.
//
// THE THEME IS voe_ui_theme_default_inputs DERIVED, in Oxanium (0185): a game
// reads no project text, so it has no theme file (0194, 0259).
//
// No keyboard is handed to ui: a game has no text field. Enter and Escape are
// the project's to read from platform.
//
// Constraints: one frame holds VOE_GAME_INTERFACE_NODES nodes and
// VOE_GAME_INTERFACE_ELEMENTS element records, no scroll areas; a frame that
// wants more is refused by voe_ui_frame_end. The context lives in the arena
// handed to _new and goes with it; the font is released by _destroy.
#pragma once

#include <game/project.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <math/float2.h>

#include <platform/window.h>

#include <render/device.h>

#include <ui/layout.h>

#include <stdbool.h>

#define VOE_GAME_SURFACE_HIGH 135.0f
// A few panels of short labels: tens of nodes each.
#define VOE_GAME_INTERFACE_NODES 256
// A glyph is an element record, and a panel and a button two each, so a
// screen of short labels is hundreds; room for several such screens.
#define VOE_GAME_INTERFACE_ELEMENTS 4096

typedef struct voe_game_interface voe_game_interface;

// The Oxanium font on `device`, the default theme derived with it, and a
// context of the two capacities in `arena`, font and theme set. NULL, with a
// line on stderr, when the font is refused.
[[nodiscard]] voe_game_interface *
voe_game_interface_new(voe_render_device *device, voe_base_arena *arena);

// Releases the font. The rest goes with the arena handed to _new.
void voe_game_interface_destroy(voe_game_interface *interface);

// The surface in millimetres for a window of `size` pixels. A size with no
// area is the caller's bug and asserts.
voe_math_float2 voe_game_interface_surface(voe_platform_size size);

// Begins the ui frame in `frame_arena`, sets the pointer (not over when
// `window` is NULL), calls `project_interface` with `asks` in the frame and
// returns its answer: false ends the run. `project_interface` ends the frame
// and may write `asks`, which is never NULL.
[[nodiscard]] bool voe_game_interface_run(
	voe_game_interface *interface, voe_base_arena *frame_arena,
	voe_ecs_world *world, voe_platform_window *window,
	voe_platform_size size, voe_game_project_asks *asks,
	bool (*project_interface)(const voe_game_project_frame *frame));

// The context the frame was laid out in, for its element records, and for a
// starting frame (game/starting.h) to lay its line out in.
voe_ui_context *voe_game_interface_context(voe_game_interface *interface);
