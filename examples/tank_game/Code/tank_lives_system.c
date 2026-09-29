// The lives' step and HUD: registers tank_lives, adds it to the first hull,
// takes a life for each shot that hit that hull this step, and draws the
// count at the top left.
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

#include <scene/parent_component.h>

#include <ui/widgets.h>

#include <stdio.h>

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

void tank_lives_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the lives in no world");
	voe_ecs_world *world = step->world;
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_lives_key);
	const voe_ecs_type hull_type =
		voe_ecs_component_type(world, &tank_hull_key);

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

bool tank_lives_interface(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->ui != NULL &&
				frame->world != NULL,
			"drawing the lives in no frame");
	const voe_ecs_type type =
		voe_ecs_component_type(frame->world, &tank_lives_key);

	if (voe_ecs_component_count(frame->world, type) == 0) {
		(void)voe_ui_frame_end(frame->ui);
		return true;
	}
	const tank_lives *row = voe_ecs_component_rows(frame->world, type);
	char text[32];

	(void)snprintf(text, sizeof(text), "Lives %d", (int)row->lives);
	voe_ui_column_begin(frame->ui, (voe_ui_container){
		.size = { .along = { VOE_UI_SIZE_FIXED, frame->size.y },
			  .across = { VOE_UI_SIZE_FIXED, frame->size.x } },
		.along = VOE_UI_ALONG_START,
		.across = VOE_UI_ACROSS_START,
		.pad = { 4.0f, 4.0f, 4.0f, 4.0f } });
	voe_ui_panel_begin(frame->ui, "lives", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){ .pad = { 4.0f, 4.0f, 4.0f,
							4.0f } });
	voe_ui_label(frame->ui, text);
	voe_ui_end(frame->ui);
	voe_ui_end(frame->ui);
	(void)voe_ui_frame_end(frame->ui);
	return true;
}
