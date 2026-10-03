// The lives' step: registers tank_lives, adds it to the first hull, makes
// that hull solid and takes a life for each shot that hit it this step. The
// menu's HUD shows the count (tank_menu.c).
//
// The collider: each step the first hull has none, a box of (4.64, 4, 2.62),
// the enemy's, not a trigger, is queued onto it, lives row or not (0295). It
// is added here, not authored, because the sponsor's tank_body.prefab has
// none and agents do not edit it; added at run time it is never saved. A
// collider already on the hull is kept.
//
// The shots that hit this step are still there: the shell system records the
// hit and its shell is only gone at the step's structural apply (0294), so
// this runs after the shells in the same step. The whole row is written.
//
// Constraints: the runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). A naive scan of
// every shot row, at most TANK_SHELL_ROWS. A full structural queue tries
// again next step.
#include "tank_lives.h"
#include "tank_hull.h"
#include "tank_shell.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <physics/collider_component.h>

#include <scene/parent_component.h>

#define TANK_LIVES_START 3

const struct voe_ecs_key tank_lives_key = { "tank_lives" };

bool tank_lives_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the lives in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_lives_key, sizeof(tank_lives), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

// How many shots hit `hull` or anything under it this step.
static int32_t tank_lives_hits(voe_ecs_world *world, voe_ecs_entity hull)
{
	VOE_BASE_ASSERT(world != NULL, "counting hits in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_shot_key);
	const uint32_t count = voe_ecs_component_count(world, type);
	const tank_shot *shots = voe_ecs_component_rows(world, type);
	int32_t hits = 0;

	VOE_BASE_ASSERT(count <= TANK_SHELL_ROWS, "more shots than rows");
	for (uint32_t i = 0; i < count; i++)
		if (shots[i].hit &&
		    voe_scene_parent_within(world, shots[i].target, hull))
			hits++;
	VOE_BASE_DEBUG_ASSERT(hits >= 0 && (uint32_t)hits <= count,
			      "more hits than shots");
	return hits;
}

// Queues the enemy's solid box onto `hull` when it has no collider, so an
// enemy's sweep has something to hit (0295). One the sponsor put there is
// kept. A refused add is left for the next step.
static void tank_lives_solidify(voe_ecs_world *world, voe_ecs_entity hull)
{
	VOE_BASE_ASSERT(world != NULL, "making a hull solid in no world");
	if (voe_physics_collider_get(world, hull) != NULL)
		return;
	const voe_physics_collider box = {
		.kind = VOE_PHYSICS_COLLIDER_BOX,
		.size = { 4.64f, 4.0f, 2.62f },
		.trigger = false,
	};

	VOE_BASE_DEBUG_ASSERT(box.size.x > 0.0f && box.size.y > 0.0f &&
				      box.size.z > 0.0f,
			      "a hull box with no size");
	(void)voe_ecs_structure_add(
		world, voe_ecs_component_type(world, &voe_physics_collider_key),
		hull, &box);
}

void tank_lives_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the lives in no world");
	voe_ecs_world *world = step->world;
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_lives_key);
	const voe_ecs_type hull_type =
		voe_ecs_component_type(world, &tank_hull_key);

	if (voe_ecs_component_count(world, hull_type) > 0)
		tank_lives_solidify(
			world, voe_ecs_component_entities(world, hull_type)[0]);
	if (voe_ecs_component_count(world, type) == 0) {
		if (voe_ecs_component_count(world, hull_type) == 0)
			return;
		const tank_lives first = { .lives = TANK_LIVES_START };

		(void)voe_ecs_structure_add(
			world, type, voe_ecs_component_entities(world, hull_type)[0],
			&first);
		return;
	}
	const voe_ecs_entity entity = voe_ecs_component_entities(world, type)[0];
	const tank_lives *now = voe_ecs_component_rows(world, type);
	const int32_t hits = tank_lives_hits(world, entity);
	const tank_lives row = { .lives = hits >= now->lives ? 0 :
							       now->lives - hits };
	const bool written = voe_ecs_component_set(world, type, entity, &row);

	VOE_BASE_ASSERT(written, "the lives row went missing");
	VOE_BASE_DEBUG_ASSERT(row.lives >= 0, "lives below zero");
}
