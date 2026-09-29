// Tank enemy: a spawned enemy tank. It drives along its own -Z and is
// removed, its turret with it, when its life runs out.
//
//     tank_enemy_register(world);          // in voe_game_project_register
//     tank_enemy_system_run(step);         // after the spawner, before the move
//
// `speed` is metres a second, default 2; `life` is seconds left, default 20,
// counted down by the system. An enemy needs a transform to drive and is a
// root, as a spawned prefab's root is.
//
// Constraints: at most TANK_ENEMY_ROWS rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

#define TANK_ENEMY_ROWS 64

#define TANK_ENEMY_FIELDS(F, F_READ_ONLY) \
	F(float, speed, FLOAT32)          \
	F(float, life, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_enemy, TANK_ENEMY_FIELDS)

extern const struct voe_ecs_key tank_enemy_key;

// Registers the type under "Tank / Enemy" with its defaults. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_enemy_register(voe_ecs_world *world);

// Drives every enemy along its own -Z for this step's seconds, counts its
// life down, and removes it and its tree when that reaches zero.
void tank_enemy_system_run(const voe_game_project_step *step);
