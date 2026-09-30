// Tank shell: a shot in flight. It flies along its own -Z and is removed
// when its life runs out or it hits something.
//
//     tank_shell_register(world);          // in voe_game_project_register
//     tank_shell_fire(step, "shell", at, turned, tank, gun_at); // a gun
//     tank_shell_system_run(step);         // after the gun, before the move
//
// `speed` is metres a second, default 30; `life` is seconds left, default 3,
// counted down by the system; `radius` is metres, default 0.1. A shell needs a
// transform to fly and is a root, as a spawned prefab's root is.
//
// EACH STEP A SHELL SWEEPS (0294 point 2): a sphere of `radius` from its
// shot's `from` to where it would fly, against the solid colliders gathered
// once that step, past its `owner`. On a hit it stops: not moved, its shot
// records `hit` and `target`, and it is removed. Only the player's shots
// swap a target with a tank_breakable row for its wreck (tank_breakable.h,
// 0296); any other shell stops and changes nothing. A hit is heard at its
// point, `Assets/sounds/hit.wav`, or, when it swaps, as
// `Assets/sounds/explosion.wav` where the broken thing stood.
//
// A SHOT CARRIES A tank_shot ROW (0294 point 1), runtime-only: never saved,
// never in the Inspector, no menu. The one firing adds it, through
// tank_shell_fire at the spawn, so it lands at the same structural apply as
// the shell; after that this system is its one writer. `owner` is the firing
// tank's root, which its sweeps ignore. `from` is where the next sweep
// starts: at the spawn the firing gun's world position, not the muzzle, so a
// muzzle already past a wall still hits the wall. `hit` and `target` record
// what the shot struck.
//
// Constraints: at most TANK_SHELL_ROWS rows of each. A refused shot row
// leaves the spawned shell flying without one.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <math/double3.h>
#include <math/quat.h>

#include <stdbool.h>

#define TANK_SHELL_ROWS 256

#define TANK_SHELL_FIELDS(F, F_READ_ONLY) \
	F(float, speed, FLOAT32)          \
	F(float, life, FLOAT32)           \
	F(float, radius, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_shell, TANK_SHELL_FIELDS)

typedef struct {
	voe_ecs_entity owner;
	voe_math_double3 from;
	bool hit;
	voe_ecs_entity target;
} tank_shot;

extern const struct voe_ecs_key tank_shell_key;
extern const struct voe_ecs_key tank_shot_key;

// Registers the shell under "Tank / Shell" with its defaults, and tank_shot
// runtime-only with no menu. False, reported, when game refuses either.
[[nodiscard]] bool tank_shell_register(voe_ecs_world *world);

// Spawns `prefab` at `position` and `rotation` and queues a tank_shot row of
// `owner` and `from` onto its root. False when the spawn or the row is
// refused.
[[nodiscard]] bool tank_shell_fire(const voe_game_project_step *step,
				   const char *prefab,
				   voe_math_double3 position,
				   voe_math_quat rotation, voe_ecs_entity owner,
				   voe_math_double3 from);

// Counts every shell's life down and removes it and its tree when that
// reaches zero; sweeps the rest, stopping and removing a shell that hits and
// swapping a breakable target for its wreck, and moves one that does not
// along its own -Z for this step's seconds.
void tank_shell_system_run(const voe_game_project_step *step);
