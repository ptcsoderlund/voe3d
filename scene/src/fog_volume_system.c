// The fog volume system: registration, the one direct creation call, and the
// run that drains whole rows, keeps the drifts one to a row and moves them by
// the wind. See scene/fog_volume_system.h for what a run does and in which
// order.
//
// A DRIFT IS CHANGED AS A COPY, set back whole: the table hands out no
// writable row. Sixteen bytes a row a step.
//
// A refused row says so once per intent on stderr, naming the entity by its
// index and generation as the clouds' drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/fog_volume_system.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define DEGREES_TO_RADIANS (3.14159265358979323846 / 180.0)

// One intent per volume per step is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key fog_volume_intent_key = {
	"voe_scene_fog_volume_intent"
};

static const voe_base_struct_description *fog_volume_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_fog_volume_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_fog_volume_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;
	const voe_scene_fog_volume row = {
		.size = { 40.0f, 10.0f, 40.0f }, .thickness = 0.4f,
		.patchiness = 0.6f, .wind_direction = 0.0f,
		.wind_speed = 1.0f, .godrays_strength = 1.0f
	};

	VOE_BASE_ASSERT(world != NULL, "registering fog volumes in no world");

	// Asserts when no transform was registered: a volume is placed by one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_fog_volume_key,
					  sizeof(voe_scene_fog_volume), capacity,
					  fog_volume_description());
	intent = voe_ecs_intent_register(world, &fog_volume_intent_key,
					 sizeof(voe_scene_fog_volume_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_scene_fog_volume_intent, fog_volume));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Rendering / Fog volume");
	(void)voe_ecs_component_register(world, &voe_scene_fog_volume_drift_key,
					 sizeof(voe_scene_fog_volume_drift),
					 capacity, &voe_ecs_runtime_only);
}

// 0 to 1 inclusive; written so a NaN fails it.
static bool unit_valid(float value)
{
	return value >= 0.0f && value <= 1.0f;
}

// Finite and not negative; written so a NaN fails it.
static bool extent_valid(float value)
{
	return isfinite(value) && value >= 0.0f;
}

static bool fog_volume_valid(voe_scene_fog_volume fog)
{
	return extent_valid(fog.size.x) && extent_valid(fog.size.y) &&
	       extent_valid(fog.size.z) && unit_valid(fog.thickness) &&
	       unit_valid(fog.patchiness) && isfinite(fog.wind_direction) &&
	       extent_valid(fog.wind_speed) && unit_valid(fog.godrays_strength);
}

bool voe_scene_fog_volume_add(voe_ecs_world *world, voe_ecs_entity entity,
			      voe_scene_fog_volume fog_volume)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a fog volume to no world");
	VOE_BASE_DEBUG_ASSERT(fog_volume_valid(fog_volume),
			      "a fog volume with a number not finite or out of bounds");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_fog_volume_key),
		entity, &fog_volume);
}

bool voe_scene_fog_volume_submit(voe_ecs_world *world,
				 voe_scene_fog_volume_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "submitting a fog volume to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &fog_volume_intent_key),
		&intent);
}

static void drain_replaces(voe_ecs_world *world)
{
	voe_ecs_intent queue = voe_ecs_intent_type(world, &fog_volume_intent_key);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_fog_volume_key);
	const voe_scene_fog_volume_intent *intents =
		voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!fog_volume_valid(intents[i].fog_volume)) {
			fprintf(stderr,
				"error: fog volume %uv%u: a number not finite or out of bounds, refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].fog_volume);
	}
	voe_ecs_intent_clear(world, queue);
}

// Drops the drifts whose row is gone, walking back so a swapped-in drift has
// been seen, then adds one at 0 to each row lacking it. Asserts the table has
// room, since it is registered with the volumes' capacity.
static void add_and_drop_drifts(voe_ecs_world *world)
{
	voe_ecs_type drifts =
		voe_ecs_component_type(world, &voe_scene_fog_volume_drift_key);
	const voe_ecs_entity *owners = voe_ecs_component_entities(world, drifts);
	const voe_ecs_entity *entities = voe_scene_fog_volume_entities(world);
	uint32_t volume_count = voe_scene_fog_volume_count(world);
	const voe_scene_fog_volume_drift start = { 0 };

	for (uint32_t i = voe_ecs_component_count(world, drifts); i > 0; i--)
		if (voe_scene_fog_volume_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, drifts,
						       owners[i - 1]);

	for (uint32_t i = 0; i < volume_count; i++) {
		if (voe_scene_fog_volume_drift_get(world, entities[i]) != NULL)
			continue;
		VOE_BASE_ASSERT(voe_ecs_component_add(world, drifts,
						      entities[i], &start),
				"the drift table is smaller than the fog volume table");
	}
}

void voe_scene_fog_volume_system_run(voe_ecs_world *world, float seconds)
{
	voe_ecs_type drifts;
	const voe_ecs_entity *owners;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the fog volume system on no world");
	VOE_BASE_ASSERT(seconds >= 0.0f, "the fog volume system run backwards");

	drain_replaces(world);
	add_and_drop_drifts(world);

	drifts = voe_ecs_component_type(world, &voe_scene_fog_volume_drift_key);
	owners = voe_ecs_component_entities(world, drifts);
	count = voe_ecs_component_count(world, drifts);
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_fog_volume *fog =
			voe_scene_fog_volume_get(world, owners[i]);
		const voe_scene_fog_volume_drift *rows =
			voe_ecs_component_rows(world, drifts);
		voe_scene_fog_volume_drift drift = rows[i];
		double angle;
		double metres;

		VOE_BASE_ASSERT(fog != NULL, "a drift outlived its fog volume");
		angle = (double)fog->wind_direction * DEGREES_TO_RADIANS;
		metres = (double)fog->wind_speed * (double)seconds;
		drift.x += sin(angle) * metres;
		drift.z -= cos(angle) * metres;
		(void)voe_ecs_component_set(world, drifts, owners[i], &drift);
	}
}
