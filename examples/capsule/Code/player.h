// Player: how fast an entity walks when the keys push it. Teaches a
// component with a default row (speed 2) and a system that reads two of the
// project's own tables and moves an engine one through its intent.
//
//     player_register(world);                   // in voe_game_project_register
//     player_system_run(world, seconds);        // after keyboard_system_run
//
// `speed` is metres a second.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define PLAYER_FIELDS(F, F_READ_ONLY) F(float, speed, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(player, PLAYER_FIELDS)

extern const struct voe_ecs_key player_key;

// Registers the type under "Player", its default speed 2. False, reported,
// when game refuses it.
[[nodiscard]] bool player_register(voe_ecs_world *world);

// For every entity with player, keyboard_input and a transform, submits the
// transform moved on XZ by move x speed x seconds: W is -Z, D is +X.
void player_system_run(voe_ecs_world *world, double seconds);
