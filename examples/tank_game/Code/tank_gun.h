// Tank gun: the part of the tank that fires. While the left mouse button or
// Space is held it spawns a prefab at its muzzle, `rate` times a second.
//
//     tank_gun_register(world);            // in voe_game_project_register
//     tank_gun_system_run(step);           // after the turret, before the move
//
// `prefab` is the name of a cooked prefab, its path under `Assets/` less
// `.prefab`, default `shell`. `rate` is shots a second, default 6. `muzzle`
// is where the shot appears in the barrel's frame, the gun's frame turned by
// its turret's `aim`, default (0, 0.3, -1.2); the shot flies along the
// barrel. `wait` is the seconds to the next shot, written only by the system.
// A gun needs a transform to fire, and sits on a turret so it fires along its
// aim.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows; `prefab` at most 63
// bytes.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <math/float3.h>

#include <stdbool.h>

#define TANK_GUN_PREFAB 64

#define TANK_GUN_FIELDS(F, F_READ_ONLY)             \
	F(char, prefab, CHAR, TANK_GUN_PREFAB)      \
	F(float, rate, FLOAT32)                     \
	F(voe_math_float3, muzzle, FLOAT3)          \
	F_READ_ONLY(float, wait, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_gun, TANK_GUN_FIELDS)

extern const struct voe_ecs_key tank_gun_key;

// Registers the type under "Tank / Gun" with its defaults. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_gun_register(voe_ecs_world *world);

// Counts every gun's wait down by the step's seconds and, while fire is held,
// spawns each ready gun's prefab at its muzzle. Does nothing with no window
// (headless).
void tank_gun_system_run(const voe_game_project_step *step);
