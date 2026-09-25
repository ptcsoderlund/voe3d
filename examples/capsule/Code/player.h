// Player: how fast an entity walks, how high it jumps and how hard it falls.
// Teaches a component with a default row and a system that reads two of the
// project's own tables and moves an engine one, the body, through its intent.
//
//     player_register(world);                   // in voe_game_project_register
//     player_system_run(world, seconds);        // after keyboard_system_run
//
// `speed` is metres a second, default 2; `jump_height` metres, default 1.2;
// `gravity` metres a second squared, default 9.81. Gravity and the jump are
// the project's, not the engine's (0249): the body only moves what it is
// given.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define PLAYER_FIELDS(F, F_READ_ONLY)   \
	F(float, speed, FLOAT32)        \
	F(float, jump_height, FLOAT32)  \
	F(float, gravity, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(player, PLAYER_FIELDS)

extern const struct voe_ecs_key player_key;

// Registers the type under "Player" with its defaults. False, reported, when
// game refuses it.
[[nodiscard]] bool player_register(voe_ecs_world *world);

// For every entity with player, keyboard_input and a body, submits the body
// whole with the velocity it wants this step: the walk on XZ, move x speed,
// W is -Z and D is +X; on Y the jump or the fall, less gravity x seconds.
void player_system_run(voe_ecs_world *world, double seconds);
