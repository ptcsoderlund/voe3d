// Tank breakable: a thing the player's shell wrecks. When the sweep of a
// shell the player fired hits an entity with this row, the shell system
// (tank_shell.h) spawns the `wreck` prefab at that entity's world place and,
// only when the spawn is not refused, removes the entity with its tree (0294
// point 3, 0296). Once a step, however many shells hit it. An enemy's shell
// stops on it and nothing changes.
//
//     tank_breakable_register(world);      // in voe_game_project_register
//
// `wreck` is the name of a cooked prefab, its path under `Assets/` less
// `.prefab`, default empty. A wreck carries no breakable, so a shell stops
// on it and nothing changes. There is no system: the shell system swaps.
// `points` is what the player scores for wrecking it, default 100: the shot
// that swaps it carries them (tank_shell.h, 0334 point 4).
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows; `wreck` at most 63
// bytes. An empty or unterminated `wreck` is a refused spawn, so the thing
// stays.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define TANK_BREAKABLE_WRECK 64

#define TANK_BREAKABLE_FIELDS(F, F_READ_ONLY) \
	F(char, wreck, CHAR, TANK_BREAKABLE_WRECK) \
	F(int32_t, points, INT32)

VOE_BASE_DESCRIBE_STRUCT(tank_breakable, TANK_BREAKABLE_FIELDS)

extern const struct voe_ecs_key tank_breakable_key;

// Registers the type under "Tank / Breakable" with an empty wreck and 100
// points. False,
// reported, when game refuses it.
[[nodiscard]] bool tank_breakable_register(voe_ecs_world *world);
