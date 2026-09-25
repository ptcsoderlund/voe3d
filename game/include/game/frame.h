// One frame of the game: the world's systems run in the editor's order, then
// the world drawn into the window through the scene's own camera, in one pass.
//
//     voe_app_frame frame = voe_app_frame_open(app);
//     if (frame.closing)
//             break;
//     if (!frame.minimised &&
//         !voe_game_frame(app, world, &shapes, scratch, frame.size))
//             ...                        // the device stopped answering
//
// THE ORDER: the structural queue applied, then the project's replaces
// (game/project.h), then the transform, identity and
// light systems, then the shape system, then a draw opened, one pass onto the
// window with voe_3d_draw_system_frame's camera and sun, the draw system, the
// pass and the draw closed. No outline, gizmo or marker: those are the
// editor's, and the frame comes back with them zeroed.
//
// Constraints: the world is one voe_game_world_new made, with exactly one
// camera and at most one light; a world with none draws every surface in its
// material colour, unshaded (0238). The device
// was opened with VOE_GAME_CAPACITIES and the shapes uploaded onto it.
// `scratch` is rewound by the draw system and keeps nothing. Called outside a
// draw; a minimised window is the caller's to skip.
#pragma once

#include <3d/shape_system.h>

#include <app/app.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <game/world.h>

#include <platform/window.h>

#include <stdbool.h>

// What the device must hold for one frame: the built-in shapes' geometry and
// their two records, one object per drawn entity in the one pass, and one pass.
// Nothing transient, no elements and no targets: the game draws no outline,
// no interface and nothing off screen.
#define VOE_GAME_CAPACITIES                                   \
	(voe_render_capacities)                               \
	{                                                     \
		.vertices = VOE_3D_SHAPES_VERTICES,            \
		.indices = VOE_3D_SHAPES_INDICES,              \
		.geometries = VOE_3D_SHAPES_GEOMETRIES,        \
		.objects = VOE_GAME_WORLD_MAX_DRAWN,           \
		.shadings = VOE_3D_SHAPES_SHADINGS, .passes = 1 \
	}

// Runs the systems and draws the world at `size`. True when the frame was
// drawn or there was nothing to draw into; false when the device refused the
// draw, the pass or the present, with render's line on stderr — the program
// should stop.
[[nodiscard]] bool voe_game_frame(voe_app *app, voe_ecs_world *world,
				  const voe_3d_shapes *shapes,
				  voe_base_arena *scratch,
				  voe_platform_size size);
