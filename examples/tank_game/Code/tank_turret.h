// Tank turret: the part of the tank that aims, as the control row
// (tank_control.h) says. Each step it turns about the world's up toward the
// point on the ground under the mouse pointer, or, on the pad, toward the
// right stick's direction on screen laid on the ground; a stick let go holds.
//
//     tank_turret_register(world);                        // in voe_game_project_register
//     tank_turret_system_run(world, window, seconds);     // after the hull, before the move
//
// `turn` is degrees a second, default 180. `aim` is the degrees about the
// turret's own up from its -Z to where its barrel points, default 0: 180 fits
// a model that faces +Z, as `tank_head.glb` does. A turret needs a transform
// to aim, and the world a camera with one to aim through.
//
// THE SYSTEM TURNS THE PLAYER'S TURRETS ONLY: those within the control row's
// hull. Any other turret is its owner's to turn, with tank_turret_turn_toward
// (0297).
//
// Constraints: at most VOE_GAME_WORLD_MAX_DRAWN rows, the drawn room, because
// spawned enemies carry turrets.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <math/double3.h>
#include <math/quat.h>

#include <platform/window.h>

#include <stdbool.h>

#define TANK_TURRET_FIELDS(F, F_READ_ONLY) \
	F(float, turn, FLOAT32)       \
	F(float, aim, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_turret, TANK_TURRET_FIELDS)

extern const struct voe_ecs_key tank_turret_key;

// Registers the type under "Tank / Turret" with its default. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_turret_register(voe_ecs_world *world);

// Turns `turret` about +Y toward facing `at`, flattened to its height, with
// its barrel, by at most its turn for `seconds`, through a transform intent.
// Writes `barrel`, the barrel's world rotation after that turn, and `left`,
// the degrees still between the barrel and the way to `at`, at or above 0.
// False, nothing queued or written, with no turret row or transform, a flat
// way nought, a barrel straight up or down, or a full queue.
[[nodiscard]] bool tank_turret_turn_toward(voe_ecs_world *world,
					   voe_ecs_entity turret,
					   voe_math_double3 at, double seconds,
					   voe_math_quat *barrel, float *left);

// For every turret within the control row's hull, turns it toward the
// pointer's aim or the pad's by at most its turn for this step's seconds. Does nothing with
// no control row (headless, or before its first step), no camera, on the
// pointer no pointer over the window, or on the pad no aim.
void tank_turret_system_run(voe_ecs_world *world, voe_platform_window *window,
			    double seconds);
