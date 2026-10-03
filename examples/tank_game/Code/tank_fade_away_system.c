// The tank fade away system: each row's `age` grown by the step, every model
// on its entity's tree faded to match, and the entity removed with its tree
// once the fade ends (tank_fade_away.h). The models are the 3d folder's, so
// they change only through voe_3d_model_submit, the whole row with its fade
// replaced; the fade away's own row is written whole through
// voe_ecs_component_set, as the light fade's is.
//
// Constraints: the tree is read into a local array of TANK_FADE_AWAY_TREE
// entities; a bigger tree fades only its first ones. A full model queue loses
// that step's fade, not the age. A refused removal is tried again next step.
#include "tank_fade_away.h"
#include "tank_enemy.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <3d/model_component.h>

#include <scene/parent_component.h>

#include <math.h>

// The most entities of one tree faded: a wreck is a hull and a turret or so.
#define TANK_FADE_AWAY_TREE 32

const struct voe_ecs_key tank_fade_away_key = { "tank_fade_away" };

static const tank_fade_away tank_fade_away_default = {
	.wait = 2.0f,
	.seconds = 1.0f,
	.age = 0.0f,
};

bool tank_fade_away_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank fade away in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_fade_away_key, sizeof(tank_fade_away), TANK_ENEMY_ROWS,
		VOE_GAME_PROJECT_DESCRIPTION(tank_fade_away),
		&tank_fade_away_default, "Tank / Fade away" });
}

// The fade a row shows: (age − wait) / seconds clamped to 0..1; with no
// seconds, gone once the wait is over.
static float faded(const tank_fade_away *row)
{
	VOE_BASE_ASSERT(row != NULL, "fading no thing");
	const float fade =
		row->seconds > 0.0f
			? fminf(1.0f, fmaxf(0.0f, (row->age - row->wait) /
							  row->seconds))
			: (row->age >= row->wait ? 1.0f : 0.0f);

	VOE_BASE_DEBUG_ASSERT(fade >= 0.0f && fade <= 1.0f,
			      "a fade outside 0..1");
	return fade;
}

// Submits every model on `root`'s tree whose fade differs, with `fade`. A
// full queue loses this step's fade.
static void fade_tree(voe_ecs_world *world, voe_ecs_entity root, float fade)
{
	VOE_BASE_ASSERT(world != NULL, "fading a tree in no world");
	voe_ecs_entity tree[TANK_FADE_AWAY_TREE];
	const uint32_t count =
		voe_scene_parent_tree(world, root, tree, TANK_FADE_AWAY_TREE);

	VOE_BASE_ASSERT(count <= TANK_FADE_AWAY_TREE,
			"a tree longer than the room given it");
	for (uint32_t i = 0; i < count; i++) {
		const voe_3d_model *model = voe_3d_model_get(world, tree[i]);

		if (model == NULL || model->fade == fade)
			continue;
		voe_3d_model next = *model;

		next.fade = fade;
		(void)voe_3d_model_submit(world,
					  (voe_3d_model_intent){
						  .entity = tree[i],
						  .model = next });
	}
}

void tank_fade_away_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"fading things away in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "fading things back in time");
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_fade_away_key);
	const tank_fade_away *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);

	VOE_BASE_ASSERT(count <= TANK_ENEMY_ROWS,
			"more tank fade away rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_fade_away next = rows[i];

		next.age += (float)step->seconds;
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank fade away row vanished while stepping it");
		fade_tree(step->world, entities[i], faded(&next));
		if (next.age >= next.wait + next.seconds)
			(void)voe_game_project_remove(step, entities[i]);
	}
}
