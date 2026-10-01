// The water system's run: the replace drain, the waves rows added and dropped,
// and the clock step. See 3d/water_system.h for what a run does and in which
// order.
//
// A WAVES ROW IS CHANGED AS A COPY, set back whole: the table hands out no
// writable row. Eight bytes a water a frame.
#include <3d/water_system.h>

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/intent.h>

#include <math.h>

#define WAVES_PERIOD 60.0

static voe_ecs_type water_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_water_key);
}

static voe_ecs_type waves_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_waves_key);
}

// Applies every replace in submission order; one for an entity with no water,
// or a dead one, is dropped.
static void drain_replaces(voe_ecs_world *world)
{
	voe_ecs_type type = water_type(world);
	voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;
	const voe_3d_water_intent *intents = voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		if (voe_ecs_component_get(world, type, intents[i].entity) == NULL)
			continue;
		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &intents[i].water);
	}
	voe_ecs_intent_clear(world, queue);
}

// Drops the rows whose water is gone, walking back so a swapped-in row has been
// seen, then adds one at 0 to each water lacking it. Asserts the table has
// room, since it is registered with the water's capacity.
static void add_and_drop_rows(voe_ecs_world *world)
{
	voe_ecs_type waves = waves_type(world);
	const voe_ecs_entity *owners = voe_ecs_component_entities(world, waves);
	const voe_ecs_entity *entities = voe_3d_water_entities(world);
	uint32_t water_count = voe_3d_water_count(world);
	const voe_3d_waves start = { 0 };

	for (uint32_t i = voe_ecs_component_count(world, waves); i > 0; i--)
		if (voe_3d_water_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, waves,
						       owners[i - 1]);

	for (uint32_t i = 0; i < water_count; i++) {
		if (voe_3d_waves_get(world, entities[i]) != NULL)
			continue;
		VOE_BASE_ASSERT(voe_ecs_component_add(world, waves, entities[i],
						      &start),
				"the waves table is smaller than the water table");
	}
}

void voe_3d_water_system_run(voe_ecs_world *world, float seconds)
{
	voe_ecs_type waves;
	const voe_ecs_entity *entities;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the water system on no world");
	VOE_BASE_ASSERT(seconds >= 0.0f, "the water system run backwards");

	drain_replaces(world);
	add_and_drop_rows(world);
	if (seconds == 0.0f)
		return;

	waves = waves_type(world);
	entities = voe_3d_waves_entities(world);
	count = voe_3d_waves_count(world);
	for (uint32_t i = 0; i < count; i++) {
		voe_3d_waves row = voe_3d_waves_rows(world)[i];

		row.seconds = fmod(row.seconds + (double)seconds, WAVES_PERIOD);
		VOE_BASE_DEBUG_ASSERT(row.seconds >= 0.0 &&
					      row.seconds < WAVES_PERIOD,
				      "a waves clock left [0, 60)");
		(void)voe_ecs_component_set(world, waves, entities[i], &row);
	}
}
