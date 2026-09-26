// Rotator: spins any entity about the world's Y, a coin or a decoration.
//
//     rotator_register(world);                  // in voe_game_project_register
//     rotator_system_run(world, seconds);       // before the move
//
// `degrees_per_second` is the turn rate, default 90. An entity needs a
// transform to turn. It spins only in the game, because project systems run
// only there (0241): the editor loads the code to register the type and
// never runs the systems.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>

#define ROTATOR_FIELDS(F, F_READ_ONLY) F(float, degrees_per_second, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(rotator, ROTATOR_FIELDS)

extern const struct voe_ecs_key rotator_key;

// Registers the type under "Rotator", default 90 degrees a second. False,
// reported, when game refuses it.
[[nodiscard]] bool rotator_register(voe_ecs_world *world);

// Turns every rotator with a transform by degrees_per_second x seconds.
void rotator_system_run(voe_ecs_world *world, double seconds);
