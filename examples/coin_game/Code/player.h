// Player: the entity the sponsor walks, and the numbers the sponsor tunes.
//
//     player_register(world);                   // in voe_game_project_register
//     player_system_run(world, window, seconds); // before the move
//     player_camera_run(world);                 // after the move
//
// `speed` is metres a second, default 2; `jump_height` metres, default 1.2;
// `gravity` metres a second squared, default 9.81; `camera_distance` metres
// behind the player, default 6; `start_score` the points a run begins with,
// default 1000; `score_drop` the points lost a second, default 10.
//
// Every number the sponsor sets sits here, on the one component the level
// already has, so the Inspector is the whole tuning panel and no separate
// game-numbers component appears in the level (0260 point 1). The run's own
// state, player_state, is runtime-only and never shown.
//
// THE KEYS ARE READ HERE, so a Player needs nothing else beside its body and
// transform; where it stands when the run begins is its start, the place a
// restart puts it back.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows of each.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <math/double3.h>

#include <platform/window.h>

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

// Where the player began, the restart count it last answered, and Space's
// level last step, so a held Space jumps once.
typedef struct {
	voe_math_double3 start;
	uint32_t restarts;
	bool jump_down;
} player_state;

extern const struct voe_ecs_key player_key;
extern const struct voe_ecs_key player_state_key;

// Registers player under "Player" with its defaults and the runtime-only
// player_state with no menu. False, reported, when game refuses either.
[[nodiscard]] bool player_register(voe_ecs_world *world);

// Walks, jumps and restarts every player with a body and a transform. Off
// the playing phase no walk and no jump; gravity still pulls. A NULL window
// reads no keys.
void player_system_run(voe_ecs_world *world, voe_platform_window *window,
		       double seconds);

// Places the scene's camera camera_distance behind the first player along
// the camera's own forward, its rotation kept.
void player_camera_run(voe_ecs_world *world);
