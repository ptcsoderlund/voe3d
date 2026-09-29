// The tank game's controls read into one row: what the player asks of the
// tank this step, from the keyboard and mouse or a gamepad (0292 point 7).
//
//     tank_control_register(world);   // in voe_game_project_register
//     tank_control_run(step);         // first before the move, ahead of the hull
//
// tank_control is one runtime-only row on the player's hull, added by
// tank_control_run's first step with a window and a hull through the
// structural queue, so it is read from the step after. The hull, turret and
// gun read it; never saved, never in the Inspector, no menu. ONE WRITER,
// THIS MODULE.
//
// `drive` and `turn` are -1..1, turn positive to the left (about +Y).
// `aim_x` and `aim_y` are the right stick's direction on screen, right and
// up positive, 0 and 0 when the stick is let go so the turret holds its aim.
// `fire` is held fire. `pad` is whether the pad is in use. `x` and `y` are
// the pointer where this row last saw it, for telling it moved.
//
// The pad is the lowest connected slot. Each stick goes through a radial dead
// zone of 0.2: shorter reads 0, longer reads the same direction rescaled to
// 0..1 past it. A trigger counts past 0.5. The pad is in use once touched (a
// stick past its dead zone, a trigger past 0.5, any button) and until the
// keyboard or mouse is (W/A/S/D, Space, a mouse button, the pointer moving),
// or no pad is connected; touched both in one step, the keyboard wins.
//
// Constraints: one row (capacity 1). Headless (no window), nothing is read
// and nothing changes. A full structural queue tries again next step.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

typedef struct {
	float drive;
	float turn;
	float aim_x;
	float aim_y;
	bool fire;
	bool pad;
	float x;
	float y;
} tank_control;

extern const struct voe_ecs_key tank_control_key;

// Registers tank_control runtime-only with no menu. False, reported, when
// game refuses it.
[[nodiscard]] bool tank_control_register(voe_ecs_world *world);

// Adds the row to the first hull on the first step with a window; after,
// reads the keyboard, mouse and pad into it each step.
void tank_control_run(const voe_game_project_step *step);
