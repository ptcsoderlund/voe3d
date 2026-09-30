// The seventeen registrations and the room behind them. The reasoning is in
// game/include/game/world.h; what is here is the numbers and the order, which
// is transforms first because a parent, a prefab, a shape, a model and an
// emitter need one (3d/shape_component.h, 3d/model_component.h,
// 3d/emitter_component.h, scene/prefab_system.h), and a
// collider before a body, which needs one (physics/body_system.h).
#include <game/world.h>

#include <game/project.h>

#include <3d/emitter_component.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>

#include <physics/body_system.h>
#include <physics/collider_system.h>

#include <scene/camera_system.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/prefab_system.h>
#include <scene/transform_system.h>

// Seventeen component types, nine of them with an intent queue and the emitter
// with a second, its control, each with room for a project's types and their
// replace intents behind it. The entities are room for what a game spawns while
// it runs, shells and enemies by the hundred, each a prefab's whole tree (0283
// point 11).
#define MAX_ENTITIES 4096
#define MAX_COMPONENT_TYPES (VOE_GAME_WORLD_TYPES + VOE_GAME_PROJECT_TYPES)
#define MAX_INTENT_TYPES (12 + VOE_GAME_PROJECT_TYPES)

// The structural queue (ecs/structure.h): room for a frame's Add, Delete,
// Duplicate or component change many times over, and for the rows they carry:
// a frame that spawns a volley of prefabs queues every row of every tree.
#define STRUCTURE_REQUESTS 2048
#define STRUCTURE_BYTES (256 * 1024)

// Transforms are wider than identities: an entity the engine makes for itself
// has one and no identity, and so has every spawned thing and each of its
// parts. Every transform may have a parent.
#define MAX_TRANSFORMS 1024

// A scene has exactly one camera (0218).
#define MAX_CAMERAS 1

// The draw system walks the panel table whether anything has one or not
// (3d/draw_system.h), so it is registered with room for one and nothing adds
// a row.
#define MAX_PANELS 1

voe_ecs_world *voe_game_world_new(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = MAX_ENTITIES,
					 .component_types = MAX_COMPONENT_TYPES,
					 .intent_types = MAX_INTENT_TYPES,
					 .structure_requests = STRUCTURE_REQUESTS,
					 .structure_bytes = STRUCTURE_BYTES });

	voe_scene_transform_register(world, MAX_TRANSFORMS);
	voe_scene_identity_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_parent_register(world, MAX_TRANSFORMS);
	voe_scene_prefab_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_light_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_camera_register(world, MAX_CAMERAS);
	voe_3d_mesh_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_material_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_panel_register(world, MAX_PANELS);
	voe_3d_shape_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_model_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_emitter_register(world, VOE_GAME_WORLD_EMITTERS);
	voe_physics_collider_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_physics_body_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_transform_previous_register(world, MAX_TRANSFORMS);

	return world;
}
