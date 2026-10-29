// The clouds system: registration, the one direct creation call, and the run
// that drains whole rows, keeps the drifts one to a row and moves them by the
// wind. See scene/clouds_system.h for what a run does and in which order.
//
// A DRIFT IS CHANGED AS A COPY, set back whole: the table hands out no
// writable row. Sixteen bytes a row a step.
//
// A refused row says so once per intent on stderr, naming the entity by its
// index and generation as the haze's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/clouds_system.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define DEGREES_TO_RADIANS (3.14159265358979323846 / 180.0)

// One intent per clouds row per step is the sizing this assumes, so the queue
// is as long as the table.
static const struct voe_ecs_key clouds_intent_key = { "voe_scene_clouds_intent" };

static const voe_base_struct_description *clouds_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_clouds_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_clouds_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;
	const voe_scene_clouds row = {
		.kind = VOE_SCENE_CLOUDS_FLAT, .coverage = 0.5f,
		.height = 1500.0f, .thickness = 600.0f,
		.wind_direction = 0.0f, .wind_speed = 10.0f,
		.shadow_strength = 0.6f
	};

	VOE_BASE_ASSERT(world != NULL, "registering clouds in no world");

	type = voe_ecs_component_register(world, &voe_scene_clouds_key,
					  sizeof(voe_scene_clouds), capacity,
					  clouds_description());
	intent = voe_ecs_intent_register(world, &clouds_intent_key,
					 sizeof(voe_scene_clouds_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_clouds_intent, clouds));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_menu_set(world, type, "Rendering / Clouds");
	(void)voe_ecs_component_register(world, &voe_scene_clouds_drift_key,
					 sizeof(voe_scene_clouds_drift),
					 capacity, &voe_ecs_runtime_only);
}

// Every number finite and each within its bounds; written so a NaN fails each.
static bool clouds_valid(voe_scene_clouds clouds)
{
	return clouds.kind <= VOE_SCENE_CLOUDS_VOLUMETRIC &&
	       clouds.coverage >= 0.0f && clouds.coverage <= 1.0f &&
	       isfinite(clouds.height) &&
	       isfinite(clouds.thickness) && clouds.thickness >= 0.0f &&
	       isfinite(clouds.wind_direction) &&
	       isfinite(clouds.wind_speed) && clouds.wind_speed >= 0.0f &&
	       clouds.shadow_strength >= 0.0f && clouds.shadow_strength <= 1.0f;
}

bool voe_scene_clouds_add(voe_ecs_world *world, voe_ecs_entity entity,
			  voe_scene_clouds clouds)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding clouds to no world");
	VOE_BASE_DEBUG_ASSERT(clouds_valid(clouds),
			      "clouds with a number not finite or out of bounds, or a kind past Volumetric");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key),
		entity, &clouds);
}

bool voe_scene_clouds_submit(voe_ecs_world *world,
			     voe_scene_clouds_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting clouds to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &clouds_intent_key), &intent);
}

static void drain_replaces(voe_ecs_world *world)
{
	voe_ecs_intent queue = voe_ecs_intent_type(world, &clouds_intent_key);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_clouds_key);
	const voe_scene_clouds_intent *intents =
		voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!clouds_valid(intents[i].clouds)) {
			fprintf(stderr,
				"error: clouds %uv%u: a number not finite or out of bounds, or a kind past Volumetric, refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].clouds);
	}
	voe_ecs_intent_clear(world, queue);
}

// Drops the drifts whose row is gone, walking back so a swapped-in drift has
// been seen, then adds one at 0 to each row lacking it. Asserts the table has
// room, since it is registered with the clouds' capacity.
static void add_and_drop_drifts(voe_ecs_world *world)
{
	voe_ecs_type drifts =
		voe_ecs_component_type(world, &voe_scene_clouds_drift_key);
	const voe_ecs_entity *owners = voe_ecs_component_entities(world, drifts);
	const voe_ecs_entity *entities = voe_scene_clouds_entities(world);
	uint32_t clouds_count = voe_scene_clouds_count(world);
	const voe_scene_clouds_drift start = { 0 };

	for (uint32_t i = voe_ecs_component_count(world, drifts); i > 0; i--)
		if (voe_scene_clouds_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, drifts,
						       owners[i - 1]);

	for (uint32_t i = 0; i < clouds_count; i++) {
		if (voe_scene_clouds_drift_get(world, entities[i]) != NULL)
			continue;
		VOE_BASE_ASSERT(voe_ecs_component_add(world, drifts,
						      entities[i], &start),
				"the drift table is smaller than the clouds table");
	}
}

void voe_scene_clouds_system_run(voe_ecs_world *world, float seconds)
{
	voe_ecs_type drifts;
	const voe_ecs_entity *owners;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the clouds system on no world");
	VOE_BASE_ASSERT(seconds >= 0.0f, "the clouds system run backwards");

	drain_replaces(world);
	add_and_drop_drifts(world);

	drifts = voe_ecs_component_type(world, &voe_scene_clouds_drift_key);
	owners = voe_ecs_component_entities(world, drifts);
	count = voe_ecs_component_count(world, drifts);
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_clouds *clouds =
			voe_scene_clouds_get(world, owners[i]);
		const voe_scene_clouds_drift *rows =
			voe_ecs_component_rows(world, drifts);
		voe_scene_clouds_drift drift = rows[i];
		double angle;
		double metres;

		VOE_BASE_ASSERT(clouds != NULL, "a drift outlived its clouds");
		angle = (double)clouds->wind_direction * DEGREES_TO_RADIANS;
		metres = (double)clouds->wind_speed * (double)seconds;
		drift.x += sin(angle) * metres;
		drift.z -= cos(angle) * metres;
		(void)voe_ecs_component_set(world, drifts, owners[i], &drift);
	}
}
