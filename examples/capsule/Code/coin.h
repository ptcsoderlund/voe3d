// Coin: a thing that spins and is taken when a player touches it. Teaches a
// trigger (0253): a collider that blocks nothing, which a project notices by
// asking what overlaps it, and removing an entity through the structural
// queue rather than by hand.
//
//     coin_register(world);                     // in voe_game_project_register
//     coin_system_run(world, seconds);          // after player_system_run
//
// `spin` is radians a second about the world's Y, default 2. A coin needs
// a transform to spin and a sphere or capsule collider, a trigger so the
// player passes through it, to be collected.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define COIN_FIELDS(F, F_READ_ONLY) F(float, spin, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(coin, COIN_FIELDS)

extern const struct voe_ecs_key coin_key;

// Registers the type under "Coin", its default spin 2. False, reported, when
// game refuses it.
[[nodiscard]] bool coin_register(voe_ecs_world *world);

// Turns every coin by spin x seconds and destroys each one a player overlaps.
void coin_system_run(voe_ecs_world *world, double seconds);
