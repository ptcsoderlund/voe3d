// One frame of the game: the world's systems run in the editor's order, then
// the sun's shadows and the world drawn into the window through the scene's
// own camera, and the project's interface over it.
//
//     voe_app_frame frame = voe_app_frame_open(app);
//     if (frame.closing)
//             break;
//     if (!frame.minimised &&
//         !voe_game_frame(app, world, &shapes, models, scratch, frame.size, lag,
//                         voe_game_interface_context(interface)))
//             ...                        // the device stopped answering
//
// THE ORDER: the world step — the structural queue applied, the project's
// replaces (game/project.h), the transform, identity and light systems, the
// shape system, the model system, the collider and body systems, then the
// sound system with no mixer, which applies edits and plays nothing — then a draw opened, the
// sun's shadow passes (voe_3d_draw_system_shadows, 0258), one pass onto the
// window with voe_3d_draw_system_frame's camera, sun and shadow, the draw
// system, depth cleared and the interface's element records drawn in one
// command over the surface of game/interface.h, the pass and the draw closed. The world step is its own call too, so
// a fixed step runs the same owning systems in the same order.
//
// `lag` IS HOW FAR BEHIND THE LAST FIXED STEP THE DRAW IS (0254): 1 − banked
// time / step, 0 draws what is, as a caller that does not step passes.
// `models` is the store the model rows are drawn and shadowed from; NULL
// draws no model (0277 point 3). No outline, gizmo or marker: those are the
// editor's, and the frame comes back with them zeroed.
//
// Constraints: the world is one voe_game_world_new made, with exactly one
// camera and at most one light; a world with none draws every lit surface
// black, the background colour and the interface as before (0287). The device
// was opened with VOE_GAME_CAPACITIES and the shapes and models uploaded onto it.
// `scratch` is rewound by the draw system and keeps nothing. Called outside a
// draw; a minimised window is the caller's to skip.
#pragma once

#include <3d/emitter_component.h>
#include <3d/models.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_system.h>

#include <app/app.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <game/interface.h>
#include <game/world.h>

#include <platform/window.h>

#include <ui/layout.h>

#include <stdbool.h>

// What the device must hold for one frame: the built-in shapes' geometry and
// their two records, and the model store's room beside them (3d/models.h);
// two objects per drawn entity, a shape's or a model part's, in the window
// pass and in each cascade, every caster drawn once into each; each emitter's
// VOE_3D_EMITTER_PARTICLES in the window pass alone, since particles cast no
// shadow (0298 points 6 and 8); the window pass and one shadow pass per cascade; the sun's maps at VOE_3D_SHADOW_TEXELS a side.
// the interface's element records, VOE_GAME_INTERFACE_ELEMENTS. Nothing
// transient and no targets: the game draws no outline and nothing off screen.
#define VOE_GAME_CAPACITIES                                                  \
	(voe_render_capacities)                                              \
	{                                                                    \
		.vertices = VOE_3D_SHAPES_VERTICES + VOE_3D_MODELS_VERTICES,  \
		.indices = VOE_3D_SHAPES_INDICES + VOE_3D_MODELS_INDICES,     \
		.geometries = VOE_3D_SHAPES_GEOMETRIES +                      \
			      VOE_3D_MODELS_GEOMETRIES,                       \
		.objects = 2 * VOE_GAME_WORLD_MAX_DRAWN *                     \
				   (1 + VOE_RENDER_SHADOW_CASCADES) +        \
			   VOE_GAME_WORLD_EMITTERS * VOE_3D_EMITTER_PARTICLES,  \
		.shadings = VOE_3D_SHAPES_SHADINGS + VOE_3D_MODELS_SHADINGS,  \
		.passes = 1 + VOE_RENDER_SHADOW_CASCADES,                     \
		.elements = VOE_GAME_INTERFACE_ELEMENTS,                      \
		.shadow_size = VOE_3D_SHADOW_TEXELS                           \
	}

// Drains every intent in the frame's order: the structural queue, the project's
// replaces, then every owning system. No move: that is a fixed step's.
void voe_game_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes);

// Runs the world step and draws the world at `size`, `lag` of a step back,
// its model rows from `models` (NULL for none),
// then `ui`'s element records over it; NULL, or no records, draws none. True when the frame was
// drawn or there was nothing to draw into; false when the device refused the
// draw, the pass or the present, with render's line on stderr — the program
// should stop.
[[nodiscard]] bool voe_game_frame(voe_app *app, voe_ecs_world *world,
				  const voe_3d_shapes *shapes,
				  const voe_3d_models *models,
				  voe_base_arena *scratch,
				  voe_platform_size size, float lag,
				  const voe_ui_context *ui);
