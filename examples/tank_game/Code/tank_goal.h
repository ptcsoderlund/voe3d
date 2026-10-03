// Tank goal: the level's end, placed by the sponsor (0334 point 5). The
// player wins when its hull's z reaches the goal's world z, the level running
// along −Z; `points` is added to the score then, default 1000. The state
// system reads it; there is no system of its own.
//
//     tank_goal_register(world);           // in voe_game_project_register
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows. It needs a transform:
// its place is its world z.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define TANK_GOAL_FIELDS(F, F_READ_ONLY) \
	F(int32_t, points, INT32)

VOE_BASE_DESCRIBE_STRUCT(tank_goal, TANK_GOAL_FIELDS)

extern const struct voe_ecs_key tank_goal_key;

// Registers the type under "Tank / Goal" with 1000 points, needing a
// transform. False, reported, when game refuses either.
[[nodiscard]] bool tank_goal_register(voe_ecs_world *world);
