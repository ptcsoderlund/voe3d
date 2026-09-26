// Follow Camera: keeps an entity a set distance behind another. Teaches an
// ENTITY field, which the Inspector offers as a dropdown of the authored
// entities, and a system that follows it to another entity's row.
//
//     follow_camera_register(world);            // in voe_game_project_register
//     follow_camera_system_run(world);          // after the move, each step
//
// `target` is the entity followed, none by default; `distance` is metres,
// default 6. The follower keeps its own rotation and sits at the target's
// position minus its own forward (its rotation applied to -Z) x distance.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows. Run after the move, it
// reads the target where this step's move left it.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define FOLLOW_CAMERA_FIELDS(F, F_READ_ONLY) \
	F(voe_ecs_entity, target, ENTITY)     \
	F(float, distance, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(follow_camera, FOLLOW_CAMERA_FIELDS)

extern const struct voe_ecs_key follow_camera_key;

// Registers the type under "Follow Camera", its default distance 6. False,
// reported, when game refuses it.
[[nodiscard]] bool follow_camera_register(voe_ecs_world *world);

// Submits each follower's transform behind its target. A dead target, or
// one with no transform, is skipped.
void follow_camera_system_run(voe_ecs_world *world);
