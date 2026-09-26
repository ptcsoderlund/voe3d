// Coin: a thing worth points, and the runtime-only mark of a taken one.
//
//     coin_register(world);                     // in voe_game_project_register
//
// `points` is what taking the coin adds to the score, default 100.
//
// A TAKEN COIN IS HIDDEN AND KEPT, NOT DESTROYED (0260 point 3): its Shape
// is removed and a `coin_taken` row keeps that shape and the restart count it
// was taken under, so a restart can put it back. coin_taken is runtime-only:
// never saved, never in the Inspector.
//
// Constraints: at most VOE_GAME_WORLD_AUTHORED rows of each.
#pragma once

#include <3d/shape_component.h>

#include <base/describe.h>

#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

#define COIN_FIELDS(F, F_READ_ONLY) F(int32_t, points, INT32)

VOE_BASE_DESCRIBE_STRUCT(coin, COIN_FIELDS)

// The shape a taken coin wore, and the restart count it was taken under.
typedef struct {
	voe_3d_shape shape;
	uint32_t restarts;
} coin_taken;

extern const struct voe_ecs_key coin_key;
extern const struct voe_ecs_key coin_taken_key;

// Registers coin under "Coin", default 100 points, and the runtime-only
// coin_taken with no menu. False, reported, when game refuses either.
[[nodiscard]] bool coin_register(voe_ecs_world *world);
