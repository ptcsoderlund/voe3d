// The round's step: registers tank_state, adds it at the menu to the first
// hull, and while playing scores this step's shots and ends the round, lost
// at 0 lives, won at the goal (0334 points 5 and 6).
//
// The shots that hit this step are still there: their shells are only gone at
// the step's structural apply (0294), so this runs after the shells and the
// lives in the same step. Lost is checked before won, so a hull that dies on
// the goal's line loses. The whole row is written.
//
// Constraints: the runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). A naive scan of
// every shot row, at most TANK_SHELL_ROWS. The goal is the first one with a
// transform; the hull is the row's entity. A full structural queue tries
// again next step.
#include "tank_state.h"
#include "tank_goal.h"
#include "tank_hull.h"
#include "tank_lives.h"
#include "tank_shell.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <scene/transform_component.h>

const struct voe_ecs_key tank_state_key = { "tank_state" };

bool tank_state_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the state in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_state_key, sizeof(tank_state), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

const tank_state *tank_state_get(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "reading the state in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_state_key);

	if (voe_ecs_component_count(world, type) == 0)
		return NULL;
	return voe_ecs_component_rows(world, type);
}

// The points of every shot that hit this step.
static int32_t tank_state_shot_points(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "scoring shots in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_shot_key);
	const uint32_t count = voe_ecs_component_count(world, type);
	const tank_shot *shots = voe_ecs_component_rows(world, type);
	int32_t points = 0;

	VOE_BASE_ASSERT(count <= TANK_SHELL_ROWS, "more shots than rows");
	for (uint32_t i = 0; i < count; i++)
		if (shots[i].hit)
			points += shots[i].points;
	return points;
}

// True when the lives row exists and is at 0.
static bool tank_state_out_of_lives(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "reading the lives in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_lives_key);

	if (voe_ecs_component_count(world, type) == 0)
		return false;
	const tank_lives *row = voe_ecs_component_rows(world, type);

	VOE_BASE_DEBUG_ASSERT(row->lives >= 0, "lives below zero");
	return row->lives == 0;
}

// The first goal with a transform that `hull` has reached, its z at or
// below the goal's world z; NULL when none has been, or either has no place.
static const tank_goal *tank_state_goal_reached(const voe_ecs_world *world,
						voe_ecs_entity hull)
{
	VOE_BASE_ASSERT(world != NULL, "finding the goal in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_goal_key);
	const uint32_t count = voe_ecs_component_count(world, type);
	const voe_ecs_entity *goals = voe_ecs_component_entities(world, type);
	const tank_goal *rows = voe_ecs_component_rows(world, type);

	if (voe_scene_transform_get(world, hull) == NULL)
		return NULL;
	for (uint32_t i = 0; i < count; i++) {
		if (voe_scene_transform_get(world, goals[i]) == NULL)
			continue;
		const double goal_z =
			voe_scene_transform_world(world, goals[i]).position.z;
		const double hull_z =
			voe_scene_transform_world(world, hull).position.z;

		return hull_z <= goal_z ? &rows[i] : NULL;
	}
	return NULL;
}

void tank_state_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the state in no world");
	voe_ecs_world *world = step->world;
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_state_key);
	const voe_ecs_type hull_type =
		voe_ecs_component_type(world, &tank_hull_key);

	if (voe_ecs_component_count(world, type) == 0) {
		if (voe_ecs_component_count(world, hull_type) == 0)
			return;
		const tank_state first = { .phase = TANK_PHASE_MENU, .score = 0 };

		(void)voe_ecs_structure_add(
			world, type, voe_ecs_component_entities(world, hull_type)[0],
			&first);
		return;
	}
	const voe_ecs_entity hull = voe_ecs_component_entities(world, type)[0];
	tank_state row = *(const tank_state *)voe_ecs_component_rows(world, type);

	if (row.phase != TANK_PHASE_PLAYING)
		return;
	row.score += tank_state_shot_points(world);
	if (tank_state_out_of_lives(world)) {
		row.phase = TANK_PHASE_LOST;
	} else {
		const tank_goal *goal = tank_state_goal_reached(world, hull);

		if (goal != NULL) {
			row.score += goal->points;
			row.phase = TANK_PHASE_WON;
		}
	}
	const bool written = voe_ecs_component_set(world, type, hull, &row);

	VOE_BASE_ASSERT(written, "the state row went missing");
	VOE_BASE_DEBUG_ASSERT(row.phase <= TANK_PHASE_LOST, "no such phase");
}
