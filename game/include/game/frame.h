// One frame of the game: the world's systems run in the editor's order, then
// the sun's shadows and the world drawn into the window through the scene's
// own camera, and the project's interface over it.
//
//     voe_app_frame frame = voe_app_frame_open(app);
//     if (frame.closing)
//             break;
//     if (!frame.minimised &&
//         !voe_game_frame(app, world, &shapes, models, scratch, frame.size, lag,
//                         casters, voe_game_interface_context(interface)))
//             ...                        // the device stopped answering
//
// THE ORDER: the world step — the structural queue applied, the project's
// replaces (game/project.h), the transform, identity, light and light blocker
// systems, the shape system, the model system, the collider and body systems, then the
// sound system with no mixer, which applies edits and plays nothing — then a draw opened, the
// light blockers (voe_3d_draw_system_light_blockers, 0347; nothing is drawn
// for them), the point lights with their shadow slots (voe_3d_draw_system_point_lights, 0320,
// 0325), the lights after the first (voe_3d_draw_system_lights, 0357), each
// casting light's shadow passes and the lamps' one (voe_3d_draw_system_shadows,
// 0258, 0325), one pass onto the window with voe_3d_draw_system_camera's
// camera, lights, shadows, point lights and blockers, the draw
// system, depth cleared and the interface's element records drawn in one
// command over the surface of game/interface.h, the pass and the draw closed. The world step is its own call too, so
// a fixed step runs the same owning systems in the same order.
//
// `lag` IS HOW FAR BEHIND THE LAST FIXED STEP THE DRAW IS (0254): 1 − banked
// time / step, 0 draws what is, as a caller that does not step passes.
// `models` is the store the model rows are drawn and shadowed from; NULL
// draws no model (0277 point 3). `casters` is the window's bounce memory
// (3d/bounce_casters.h, 0394 point 4), one the caller keeps per window; NULL
// keeps none, and a removed caster then marks nothing. No outline, gizmo or
// marker: those are the editor's, and the frame comes back with them zeroed.
//
// Constraints: the world is one voe_game_world_new made, with exactly one
// camera; at most VOE_RENDER_DIRECTIONAL_LIGHTS lights are drawn, in table
// order, the rest left out (0357 point 1); a world with none draws every lit surface
// black, the background colour and the interface as before (0287). The device
// was opened with VOE_GAME_CAPACITIES and the shapes and models uploaded onto it.
// `scratch` is rewound by the draw system and keeps nothing. Called outside a
// draw; a minimised window is the caller's to skip.
#pragma once

#include <3d/bounce_casters.h>
#include <3d/draw_system.h>
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
// pass, in each cascade of each of VOE_RENDER_DIRECTIONAL_LIGHTS lights, in the
// point-shadow pass, in each of the bounce's capture passes and in each bounce
// sun map, every caster drawn at most once into each; VOE_3D_LANDSCAPE_NODES
// for each of VOE_3D_LANDSCAPES_DRAWN landscape rows in each of those same
// passes (0396 point 4); each emitter's
// VOE_3D_EMITTER_PARTICLES in the window pass alone, since particles cast no
// shadow (0298 points 6 and 8); one per water, VOE_GAME_WORLD_WATERS, in the
// window pass alone, since water casts no shadow (0305 point 7); the window
// pass, one shadow pass per cascade per casting light (0357 point 3), the
// lamps' point-shadow pass (0325 point 7), and up to
// VOE_RENDER_BOUNCE_CAPTURE_PASSES capture passes after it, the bounce map
// pass gone (0326 points 3 and 8), then one bounce sun map per volume per
// casting sun, VOE_RENDER_BOUNCE_VOLUMES × VOE_RENDER_DIRECTIONAL_LIGHTS, on a
// frame that relights (0329 point 3, 0357 point 4, 0389); the sun's maps at VOE_3D_SHADOW_TEXELS a side and the lamps'
// faces at VOE_3D_POINT_SHADOW_TEXELS; the interface's element records, VOE_GAME_INTERFACE_ELEMENTS. Nothing
// transient and no targets: the game draws no outline and nothing off screen.
// No heights_texels: the game never sculpts, and its landscapes are uploaded
// whole at load.
#define VOE_GAME_CAPACITIES                                                  \
	(voe_render_capacities)                                              \
	{                                                                    \
		.vertices = VOE_3D_SHAPES_VERTICES + VOE_3D_MODELS_VERTICES,  \
		.indices = VOE_3D_SHAPES_INDICES + VOE_3D_MODELS_INDICES,     \
		.geometries = VOE_3D_SHAPES_GEOMETRIES +                      \
			      VOE_3D_MODELS_GEOMETRIES,                       \
		.objects = (2 * VOE_GAME_WORLD_MAX_DRAWN +                    \
			    VOE_3D_LANDSCAPES_DRAWN * VOE_3D_LANDSCAPE_NODES) * \
				   (2 + VOE_RENDER_DIRECTIONAL_LIGHTS *       \
						VOE_RENDER_SHADOW_CASCADES +       \
					    VOE_RENDER_BOUNCE_VOLUMES *              \
						VOE_RENDER_DIRECTIONAL_LIGHTS +    \
					    VOE_RENDER_BOUNCE_CAPTURE_PASSES) +      \
			   VOE_GAME_WORLD_EMITTERS * VOE_3D_EMITTER_PARTICLES + \
			   VOE_GAME_WORLD_WATERS,                               \
		.shadings = VOE_3D_SHAPES_SHADINGS + VOE_3D_MODELS_SHADINGS,  \
		.passes = 1 + VOE_RENDER_SHADOW_CASCADES *                    \
				      VOE_RENDER_DIRECTIONAL_LIGHTS +         \
			  1 + VOE_RENDER_BOUNCE_CAPTURE_PASSES +              \
			  VOE_RENDER_BOUNCE_VOLUMES *                         \
				  VOE_RENDER_DIRECTIONAL_LIGHTS,              \
		.elements = VOE_GAME_INTERFACE_ELEMENTS,                      \
		.shadow_size = VOE_3D_SHADOW_TEXELS,                          \
		.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS               \
	}

// Drains every intent in the frame's order: the structural queue, the project's
// replaces, then every owning system. No move: that is a fixed step's.
void voe_game_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes);

// Runs the world step and draws the world at `size`, `lag` of a step back,
// its model rows from `models` (NULL for none), its bounce remembering its
// casters in `casters`, the caller's one for this window (NULL for none: a
// removed caster then marks nothing),
// then `ui`'s element records over it; NULL, or no records, draws none. True when the frame was
// drawn or there was nothing to draw into; false when the device refused the
// draw, the pass or the present, with render's line on stderr — the program
// should stop.
[[nodiscard]] bool voe_game_frame(voe_app *app, voe_ecs_world *world,
				  const voe_3d_shapes *shapes,
				  const voe_3d_models *models,
				  voe_base_arena *scratch,
				  voe_platform_size size, float lag,
				  voe_3d_bounce_casters *casters,
				  const voe_ui_context *ui);
