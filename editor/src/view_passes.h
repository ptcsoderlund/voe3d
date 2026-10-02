// What a frame draws into the scene views: a pass per view the dock tree
// shows, each preceded by the sun's shadow passes fitted to that view
// (voe_3d_draw_system_shadows, ADR-0258) and onto that view's own target with
// its own camera, the light voe_editor_view_light gives (the world's or the
// preview) and those shadows, the world and its models drawn by voe_3d_draw_system_run with the
// selection's outline, a model's too (ADR-0203), its collider as lines (0253), its move
// gizmo (ADR-0205), the scene camera's marker (0223), the sun's (0274) and every point light's (0320), and the pass ended; and before those, the preview's
// shadow passes and one pass with the world's own camera (view.h); every pass lit by the point lights. main.c calls both once a
// frame, between opening the draw and the window's pass:
//
//     drawn = voe_editor_view_passes_preview(gpu, arena, world, &views,
//                                            light, &scene, models);
//     drawn = drawn && voe_editor_view_passes_draw(gpu, arena, world, &views, &tree,
//                                         light, &scene, &geometries, models,
//                                         &shapes, palette, &gizmo, ppmm);
//
// A REFUSED PASS STOPS THE REST, a shadow pass as much as a view's: no later
// view is begun and the call returns false, the caller's `drawn`, and the frame
// is still the caller's to close.
//
// WHAT THE DEVICE'S CAPACITIES MUST COVER is VOE_EDITOR_CAPACITIES below, kept
// beside the passes so the next capacity a pass needs is added here. The
// editor uploads one cube and one material — the shapes' own, see
// 3d/shape_system.h — and the model store's room on top (VOE_3D_MODELS_*,
// 3d/models.h, ADR-0277). `objects` is per frame: every drawn entity is one
// object in every view's pass, and a model part is an object too, so it is
// twice the room for drawn entities (VOE_GAME_WORLD_MAX_DRAWN, game/world.h's
// — every project's world is registered with that much room for a mesh and a
// material, so a device that draws one is sized from the same number), one more for the selected
// entity's outline, which is drawn into every view's pass too, and two more
// for the gizmo's handles at rest and its marked one, one for the camera's
// marker, one for the sun's, one for the selection's collider and two for the
// point lights' markers, times the room for views, and the drawn entities once more for the
// preview's pass, which draws the world alone; and every caster once per
// cascade and once more for the bounce map, VOE_GAME_WORLD_MAX_DRAWN ×
// (VOE_RENDER_SHADOW_CASCADES + 1), per view and for the preview (0308); and every emitter's particles, VOE_GAME_WORLD_EMITTERS ×
// VOE_3D_EMITTER_PARTICLES, an object each in every view's pass and the
// preview's (0298 point 8); and every water, VOE_GAME_WORLD_WATERS, an object
// each in every view's pass and the preview's — water casts no shadow, so it
// adds nothing per cascade (0305). `passes` is a pass per view, the preview's and the
// interface's, and a shadow pass per cascade and one bounce pass for each view
// and the preview (`shadow_size` is VOE_3D_SHADOW_TEXELS, 3d/shadow_cascades.h), and
// `targets` a target per view and the preview's, each with its own probe grid
// (0308 point 3), each two texture slots now, its colour and its depth copy
// (0305 point 1) — both from the room for views, not the two in use, so a
// third view is a leaf and not a capacity. The three transient numbers are
// what the selection outline's quads are copied into: one outline per view's
// pass, sized the way `passes` and `targets` are (ADR-0203, 3d/outline.h).
// The gizmo's quads go there too: one gizmo per view's pass, two ranges and
// two draws, its share the larger of the arrows' and the rings' so either
// mode fits (ADR-0205, 0274, 3d/draw_system.h). So do the camera marker's:
// one marker per view's pass, one range and one draw, sized the same way
// (0223, 3d/camera_marker.h), the sun marker's (0274, 3d/sun_marker.h) and
// the collider's, one more of each (0253, 3d/collider_marker.h), and the point
// light markers': VOE_GAME_WORLD_POINT_LIGHTS lamps a view, in two ranges and
// draws, the rest and the selected one (0320 point 9, 3d/point_light_marker.h).
#pragma once

#include "dock.h"
#include "gizmo.h"
#include "interface.h"
#include "project.h"
#include "scene.h"
#include "view.h"

#include <3d/draw_system.h>
#include <3d/emitter_component.h>
#include <3d/models.h>
#include <3d/point_light_marker.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_geometry.h>
#include <3d/shape_system.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <render/device.h>

#include <ui/theme.h>

#include <stdbool.h>

// How thick the selection's outline is drawn, in the surface's millimetres.
// HOW WIDE THAT LINE IS IS THIS PROGRAM'S TO CHOOSE, the same as a notch's
// worth: `3d` takes a width in pixels, everything the editor sizes is in the
// surface's millimetres, and `pixels_per_millimetre` is the one multiplication
// between them — so the outline is the same thickness on a screen of any
// density (ADR-0180).
#define VOE_EDITOR_OUTLINE_MILLIMETRES 0.4f

// How long one arrow's shaft of the move gizmo is drawn, in the surface's
// millimetres, for the outline's reason: a size on the surface times
// `pixels_per_millimetre` is a size in pixels, so the gizmo is one size on a
// screen of any density — and what is hit is what is drawn (gizmo.h).
#define VOE_EDITOR_GIZMO_MILLIMETRES 12.0f

// The gizmo's share of the transient pool: the arrows or the rings, whichever
// is larger, so switching modes needs no other capacity.
#define VOE_EDITOR_GIZMO_VERTICES                                   \
	(VOE_3D_GIZMO_VERTICES > VOE_3D_GIZMO_RING_VERTICES          \
		 ? VOE_3D_GIZMO_VERTICES                               \
		 : VOE_3D_GIZMO_RING_VERTICES)
#define VOE_EDITOR_GIZMO_INDICES                                    \
	(VOE_3D_GIZMO_INDICES > VOE_3D_GIZMO_RING_INDICES            \
		 ? VOE_3D_GIZMO_INDICES                                \
		 : VOE_3D_GIZMO_RING_INDICES)

// What the device is opened with; the reasoning is at the top of this file.
#define VOE_EDITOR_CAPACITIES                                                 \
	(voe_render_capacities)                                               \
	{                                                                     \
		.vertices = VOE_3D_SHAPES_VERTICES + VOE_3D_MODELS_VERTICES,   \
		.indices = VOE_3D_SHAPES_INDICES + VOE_3D_MODELS_INDICES,      \
		.geometries =                                                  \
			VOE_3D_SHAPES_GEOMETRIES + VOE_3D_MODELS_GEOMETRIES,  \
		.objects = (2 * VOE_GAME_WORLD_MAX_DRAWN + 8) *                \
				   VOE_EDITOR_VIEWS +                          \
			   2 * VOE_GAME_WORLD_MAX_DRAWN +                      \
			   2 * VOE_GAME_WORLD_MAX_DRAWN *                      \
				   (VOE_RENDER_SHADOW_CASCADES + 1) *          \
				   (VOE_EDITOR_VIEWS + 1) +                    \
			   VOE_GAME_WORLD_EMITTERS * VOE_3D_EMITTER_PARTICLES * \
				   (VOE_EDITOR_VIEWS + 1) +                    \
			   VOE_GAME_WORLD_WATERS * (VOE_EDITOR_VIEWS + 1),     \
		.shadings = VOE_3D_SHAPES_SHADINGS + VOE_3D_MODELS_SHADINGS,   \
		.elements = VOE_EDITOR_INTERFACE_ELEMENTS,                     \
		.passes = VOE_EDITOR_VIEWS + 2 +                               \
			  (VOE_RENDER_SHADOW_CASCADES + 1) *                   \
				  (VOE_EDITOR_VIEWS + 1),                      \
		.shadow_size = VOE_3D_SHADOW_TEXELS,                           \
		.targets = VOE_EDITOR_VIEWS + 1,                               \
		.transient_vertices = (VOE_3D_OUTLINE_VERTICES +               \
				       VOE_EDITOR_GIZMO_VERTICES +             \
				       VOE_3D_CAMERA_MARKER_VERTICES +         \
				       VOE_3D_SUN_MARKER_VERTICES +            \
				       VOE_3D_COLLIDER_MARKER_VERTICES +       \
				       VOE_3D_POINT_LIGHT_MARKER_VERTICES *    \
					       VOE_GAME_WORLD_POINT_LIGHTS) *  \
				      VOE_EDITOR_VIEWS,                        \
		.transient_indices = (VOE_3D_OUTLINE_INDICES +                 \
				      VOE_EDITOR_GIZMO_INDICES +               \
				      VOE_3D_CAMERA_MARKER_INDICES +           \
				      VOE_3D_SUN_MARKER_INDICES +              \
				      VOE_3D_COLLIDER_MARKER_INDICES +         \
				      VOE_3D_POINT_LIGHT_MARKER_INDICES *      \
					      VOE_GAME_WORLD_POINT_LIGHTS) *   \
				     VOE_EDITOR_VIEWS,                         \
		.transient_geometries = 8 * VOE_EDITOR_VIEWS                   \
	}

// Sets `preview_shown` to whether the selected entity has a camera and, when
// it has and that camera is not blind, draws what it sees into the preview's
// target, lit by `light`, with `models`' rows (NULL for none) and no outline,
// gizmo or marker. False only when
// the device refuses the pass. Called inside an open draw, never inside a pass.
[[nodiscard]] bool voe_editor_view_passes_preview(
	voe_render_device *gpu, voe_base_arena *arena, voe_ecs_world *world,
	voe_editor_views *views, voe_render_light light,
	const voe_editor_scene *scene, const voe_3d_models *models);

// Draws every view `tree` shows, in order, models and the selected model's
// outline through `models` (NULL for none), and returns false at the first
// pass the device refuses. Called inside an open draw, never inside a pass.
[[nodiscard]] bool voe_editor_view_passes_draw(
	voe_render_device *gpu, voe_base_arena *arena, voe_ecs_world *world,
	const voe_editor_views *views, const voe_editor_dock_tree *tree,
	voe_render_light light, const voe_editor_scene *scene,
	const voe_3d_shape_geometries *geometries, const voe_3d_models *models,
	const voe_3d_shapes *shapes, const voe_ui_theme *palette, const voe_editor_gizmo *gizmo,
	float pixels_per_millimetre);
