// The eleven registrations and the room behind them. The reasoning is in
// game/include/game/world.h; what is here is the numbers and the order, which
// is transforms first because a shape needs one (3d/shape_component.h), and a
// collider before a body, which needs one (physics/body_system.h).
#include <game/world.h>

#include <game/project.h>

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>

#include <physics/body_system.h>
#include <physics/collider_system.h>

#include <scene/camera_system.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

// Eleven component types, six of them with an intent queue, each with room for
// a project's types and their replace intents behind it; the entities are a
// number to author into rather than a measurement of anything.
#define MAX_ENTITIES 1024
#define MAX_COMPONENT_TYPES (VOE_GAME_WORLD_TYPES + VOE_GAME_PROJECT_TYPES)
#define MAX_INTENT_TYPES (10 + VOE_GAME_PROJECT_TYPES)

// The structural queue (ecs/structure.h): room for a frame's Add, Delete,
// Duplicate or component change many times over, and for the rows they carry.
#define STRUCTURE_REQUESTS 256
#define STRUCTURE_BYTES 32768

// Transforms are wider than identities: an entity the engine makes for itself
// has one and no identity.
#define MAX_TRANSFORMS 256

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
	voe_scene_light_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_camera_register(world, MAX_CAMERAS);
	voe_3d_mesh_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_material_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_3d_panel_register(world, MAX_PANELS);
	voe_3d_shape_register(world, VOE_GAME_WORLD_MAX_DRAWN);
	voe_physics_collider_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_physics_body_register(world, VOE_GAME_WORLD_AUTHORED);
	voe_scene_transform_previous_register(world, MAX_TRANSFORMS);

	return world;
}
