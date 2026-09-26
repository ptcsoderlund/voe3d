// Player: the entity the sponsor walks, and the numbers the sponsor tunes.
//
//     player_register(world);                   // in voe_game_project_register
//
// `speed` is metres a second, default 2; `jump_height` metres, default 1.2;
// `gravity` metres a second squared, default 9.81; `camera_distance` metres
// behind the player, default 6; `start_score` the points a run begins with,
// default 1000; `score_drop` the points lost a second, default 10.
//
// Every number the sponsor sets sits here, on the one component the level
// already has, so the Inspector is the whole tuning panel and no separate
// game-numbers component appears in the level (0260 point 1). The run's own
// state is runtime-only and never shown.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

#define PLAYER_FIELDS(F, F_READ_ONLY)        \
	F(float, speed, FLOAT32)             \
	F(float, jump_height, FLOAT32)       \
	F(float, gravity, FLOAT32)           \
	F(float, camera_distance, FLOAT32)   \
	F(int32_t, start_score, INT32)       \
	F(int32_t, score_drop, INT32)

VOE_BASE_DESCRIBE_STRUCT(player, PLAYER_FIELDS)

extern const struct voe_ecs_key player_key;

// Registers the type under "Player" with its defaults. False, reported, when
// game refuses it.
[[nodiscard]] bool player_register(voe_ecs_world *world);
