// Tank enemy: a spawned enemy tank. It drives along its own -Z, fires at the
// player within range, and is removed, its turret with it, when its life runs
// out.
//
//     tank_enemy_register(world);          // in voe_game_project_register
//     tank_enemy_system_run(step);         // after the spawner, before the move
//
// `speed` is metres a second, default 2; `life` is seconds left, default 20,
// counted down by the system. An enemy needs a transform to drive and fire
// and is a root, as a spawned prefab's root is.
//
// IT FIRES AT THE PLAYER (0294 point 5): the entity of the tank_lives row
// (tank_lives.h), the player's hull; with no row nothing fires. `prefab` is
// the cooked prefab fired, default `shell`; `rate` is shots a second, default
// 0.5, and at or below 0 never fires; `range` is metres, default 30. `wait` is
// the seconds to the next shot, written only by the system; its default, 1 s,
// is the hold before a new enemy's first shot (0338), while it turns and
// drives as ever.
//
// IT AIMS ITS TURRET FIRST (0297): its turret is its child with a tank_turret
// row (tank_turret.h). With the player within range, the turret turns toward
// the player at its row's `turn`, and the enemy fires only when the barrel is
// on target; with no turret it never fires. `muzzle`, default (0, 0.5, -3), is
// in the barrel's frame about the turret's world position, and the shot
// leaves turned as the barrel. Each shot bursts the turret's emitter, the
// prefab's muzzle flash (0299 point 2); a turret with none flashes nothing.
//
// IT HUMS (0304 point 8): an enemy with no sound is given the looping engine
// hum, quieter and lower than the player's, gone with it when it is removed.
//
// Constraints: at most TANK_ENEMY_ROWS rows; `prefab` at most 63 bytes.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <math/float3.h>

#include <stdbool.h>

#define TANK_ENEMY_ROWS 64
#define TANK_ENEMY_PREFAB 64

#define TANK_ENEMY_FIELDS(F, F_READ_ONLY)          \
	F(float, speed, FLOAT32)                   \
	F(float, life, FLOAT32)                    \
	F(char, prefab, CHAR, TANK_ENEMY_PREFAB)   \
	F(float, rate, FLOAT32)                    \
	F(float, range, FLOAT32)                   \
	F(voe_math_float3, muzzle, FLOAT3)         \
	F_READ_ONLY(float, wait, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_enemy, TANK_ENEMY_FIELDS)

extern const struct voe_ecs_key tank_enemy_key;

// Registers the type under "Tank / Enemy" with its defaults. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_enemy_register(voe_ecs_world *world);

// Drives every enemy along its own -Z for this step's seconds, counts its
// life and wait down, turns each one's turret toward the player within
// range, fires each ready one whose barrel is on target, and
// removes it and its tree when its life reaches zero.
void tank_enemy_system_run(const voe_game_project_step *step);
