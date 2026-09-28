// Tank hull: the part of the tank that drives. W and S move it along its own
// forward, A and D turn it about the world's up.
//
//     tank_hull_register(world);                        // in voe_game_project_register
//     tank_hull_system_run(world, window, seconds);     // before the move
//
// `speed` is metres a second, default 4; `turn` is degrees a second,
// default 90. A hull needs a transform to drive.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <platform/window.h>

#include <stdbool.h>

#define TANK_HULL_FIELDS(F, F_READ_ONLY)   \
	F(float, speed, FLOAT32)           \
	F(float, turn, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_hull, TANK_HULL_FIELDS)

extern const struct voe_ecs_key tank_hull_key;

// Registers the type under "Tank / Hull" with its defaults. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_hull_register(voe_ecs_world *world);

// For every hull with a transform, turns it by A/D and drives it by W/S for
// this step's seconds. Does nothing with no window (headless).
void tank_hull_system_run(voe_ecs_world *world, voe_platform_window *window,
			  double seconds);
