// Tank turret: the part of the tank that aims. Each step it turns about the
// world's up toward the point on the ground under the mouse pointer.
//
//     tank_turret_register(world);                        // in voe_game_project_register
//     tank_turret_system_run(world, window, seconds);     // after the hull, before the move
//
// `turn` is degrees a second, default 180. A turret needs a transform to aim,
// and the world a camera with one to aim through.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <platform/window.h>

#include <stdbool.h>

#define TANK_TURRET_FIELDS(F, F_READ_ONLY) \
	F(float, turn, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_turret, TANK_TURRET_FIELDS)

extern const struct voe_ecs_key tank_turret_key;

// Registers the type under "Tank / Turret" with its default. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_turret_register(voe_ecs_world *world);

// For every turret with a transform, turns it toward the pointer's aim by at
// most its turn for this step's seconds. Does nothing with no window
// (headless), no pointer over it, or no camera.
void tank_turret_system_run(voe_ecs_world *world, voe_platform_window *window,
			    double seconds);
