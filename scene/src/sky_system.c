// The sky system: registration, the one direct creation call, and the drain
// that applies a whole row or keeps the row over one it refuses.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for the sun. What the sky looks like
// is the renderer's; it lights nothing.
//
// AN INTENT FOR AN ENTITY WITH NO SKY IS DROPPED, silently — see the header.
// A refused row says so once per intent on stderr, naming the entity by its
// index and generation as the sun's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/sky_system.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per sky per frame is the sizing this assumes, so the queue is as
// long as the table.
static const struct voe_ecs_key sky_intent_key = { "voe_scene_sky_intent" };

static const voe_base_struct_description *sky_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_sky_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_sky_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;
	const voe_scene_sky row = { .zenith = { 0.15f, 0.35f, 0.80f },
				    .horizon = { 0.60f, 0.75f, 0.90f },
				    .ground = { 0.20f, 0.18f, 0.16f },
				    .horizon_height = 0.25f,
				    .strength = 1.0f };

	VOE_BASE_ASSERT(world != NULL, "registering skies in no world");

	type = voe_ecs_component_register(world, &voe_scene_sky_key,
					  sizeof(voe_scene_sky), capacity,
					  sky_description());
	intent = voe_ecs_intent_register(world, &sky_intent_key,
					 sizeof(voe_scene_sky_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_sky_intent, sky));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_menu_set(world, type, "Rendering / Sky");
}

// Finite and not negative. Written so a NaN fails it.
static bool amount_valid(float value)
{
	return isfinite(value) && value >= 0.0f;
}

// Every channel finite and not negative.
static bool colour_valid(voe_math_float3 colour)
{
	return amount_valid(colour.x) && amount_valid(colour.y) &&
	       amount_valid(colour.z);
}

// Within [0, max]. Written so a NaN fails it; an infinity is past max.
static bool within(float value, float max)
{
	return value >= 0.0f && value <= max;
}

// Every bound of the header at once; written so a NaN fails each comparison.
static bool sky_valid(voe_scene_sky sky)
{
	return colour_valid(sky.zenith) && colour_valid(sky.horizon) &&
	       colour_valid(sky.ground) && within(sky.horizon_height, 1.0f) &&
	       amount_valid(sky.strength);
}

bool voe_scene_sky_add(voe_ecs_world *world, voe_ecs_entity entity,
		       voe_scene_sky sky)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a sky to no world");
	VOE_BASE_DEBUG_ASSERT(sky_valid(sky),
			      "a sky with a number out of bounds or not finite");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_sky_key),
		entity, &sky);
}

bool voe_scene_sky_submit(voe_ecs_world *world, voe_scene_sky_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a sky to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &sky_intent_key), &intent);
}

void voe_scene_sky_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_sky_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the sky system on no world");

	queue = voe_ecs_intent_type(world, &sky_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_sky_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!sky_valid(intents[i].sky)) {
			fprintf(stderr,
				"error: sky %uv%u: a number out of bounds or not finite refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].sky);
	}

	voe_ecs_intent_clear(world, queue);
}
