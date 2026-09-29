// Tank spawner: a place enemies come from. Every `every` seconds it spawns a
// prefab at itself, while the world holds fewer than `most` enemies.
//
//     tank_spawner_register(world);        // in voe_game_project_register
//     tank_spawner_system_run(step);       // after the shells, before the enemies
//
// `prefab` is the name of a cooked prefab, its path under `Assets/` less
// `.prefab`, default `enemy_tank`. `every` is seconds between spawns, default
// 4. `most` is the most `tank_enemy` rows the world holds, default 6. `wait`
// is the seconds to the next spawn, written only by the system. A spawner
// needs a transform: the spawned root takes its world position and rotation.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows; `prefab` at most 63
// bytes.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>
#include <stdint.h>

#define TANK_SPAWNER_PREFAB 64

#define TANK_SPAWNER_FIELDS(F, F_READ_ONLY)             \
	F(char, prefab, CHAR, TANK_SPAWNER_PREFAB)      \
	F(float, every, FLOAT32)                        \
	F(uint32_t, most, UINT32)                       \
	F_READ_ONLY(float, wait, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_spawner, TANK_SPAWNER_FIELDS)

extern const struct voe_ecs_key tank_spawner_key;

// Registers the type under "Tank / Spawner" with its defaults. False,
// reported, when game refuses it.
[[nodiscard]] bool tank_spawner_register(voe_ecs_world *world);

// Counts every spawner's wait down by the step's seconds and spawns each
// ready one's prefab at it while the world holds fewer than its `most`
// enemies.
void tank_spawner_system_run(const voe_game_project_step *step);
