// Tank breakable: a thing a shell wrecks. When a shell's sweep hits an
// entity with this row, the shell system (tank_shell.h) spawns the `wreck`
// prefab at that entity's world place and, only when the spawn is not
// refused, removes the entity with its tree (0294 point 3). Once a step,
// however many shells hit it.
//
//     tank_breakable_register(world);      // in voe_game_project_register
//
// `wreck` is the name of a cooked prefab, its path under `Assets/` less
// `.prefab`, default empty. A wreck carries no breakable, so a shell stops
// on it and nothing changes. There is no system: the shell system swaps.
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
	F(char, wreck, CHAR, TANK_BREAKABLE_WRECK)

VOE_BASE_DESCRIBE_STRUCT(tank_breakable, TANK_BREAKABLE_FIELDS)

extern const struct voe_ecs_key tank_breakable_key;

// Registers the type under "Tank / Breakable" with an empty wreck. False,
// reported, when game refuses it.
[[nodiscard]] bool tank_breakable_register(voe_ecs_world *world);
