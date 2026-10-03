// Tank fade away: a thing that lies still `wait` seconds, then turns
// see-through over `seconds`, every model on it and under it, then leaves the
// world with its tree, as an enemy's wreck does (0336 point 4).
//
//     tank_fade_away_register(world);           // in voe_game_project_register
//     tank_fade_away_system_run(step);          // while playing, after the enemy
//
// IT IS GAME CODE, NOT THE ENGINE'S: a model draws at its row's `fade`
// (3d/model_component.h), and a game that wants a thing gone fades that
// field itself, through the model's intent.
//
// `wait` is the seconds it lies still, default 2. `seconds` is how long it
// fades, default 1; at or below 0 it is gone at once when the wait ends.
// `age` is the seconds it has lain, default 0, written only by the system.
// Each step every model on the entity's tree shows (age − wait) / seconds,
// clamped to 0..1, and only a model whose fade differs is submitted. At `age`
// ≥ wait + seconds the entity and its tree are removed.
//
// Constraints: at most TANK_ENEMY_ROWS rows, since a wreck lives seconds and
// the enemies bound them; a tree of at most TANK_FADE_AWAY_TREE entities is
// faded, the rest left solid until removed. A full model queue loses that
// step's fade, not the age; a full structural queue tries the removal again
// next step.
#pragma once

#include <base/describe.h>

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

#define TANK_FADE_AWAY_FIELDS(F, F_READ_ONLY) \
	F(float, wait, FLOAT32)               \
	F(float, seconds, FLOAT32)            \
	F_READ_ONLY(float, age, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(tank_fade_away, TANK_FADE_AWAY_FIELDS)

extern const struct voe_ecs_key tank_fade_away_key;

// Registers the type under "Tank / Fade away" with its defaults. False,
// reported, when game refuses it.
[[nodiscard]] bool tank_fade_away_register(voe_ecs_world *world);

// Ages every fading thing by the step's seconds, fades each model on its tree
// to match, and removes each one whose fade has ended.
void tank_fade_away_system_run(const voe_game_project_step *step);
