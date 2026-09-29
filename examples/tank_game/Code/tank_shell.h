// Tank shell: a shot in flight. It flies along its own -Z and is removed
// when its life runs out.
//
//     tank_shell_register(world);          // in voe_game_project_register
//     tank_shell_system_run(step);         // after the gun, before the move
//
// `speed` is metres a second, default 30; `life` is seconds left, default 3,
// counted down by the system. A shell needs a transform to fly and is a root,
// as a spawned prefab's root is.
//
// Constraints: at most TANK_SHELL_ROWS rows.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

#define TANK_SHELL_ROWS 256

#define TANK_SHELL_FIELDS(F, F_READ_ONLY) \
	F(float, speed, FLOAT32)          \
	F(float, life, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_shell, TANK_SHELL_FIELDS)

extern const struct voe_ecs_key tank_shell_key;

// Registers the type under "Tank / Shell" with its defaults. False, reported,
// when game refuses it.
[[nodiscard]] bool tank_shell_register(voe_ecs_world *world);

// Moves every shell along its own -Z for this step's seconds, counts its
// life down, and removes it and its tree when that reaches zero.
void tank_shell_system_run(const voe_game_project_step *step);
