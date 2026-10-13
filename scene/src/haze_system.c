// The haze system: registration, the one direct creation call, and the drain
// that applies a whole row or keeps the row over one it refuses.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for the sky. What the haze looks
// like, its colour included, is the renderer's.
//
// AN INTENT FOR AN ENTITY WITH NO HAZE IS DROPPED, silently — see the header.
// A refused row says so once per intent on stderr, naming the entity by its
// index and generation as the sky's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/haze_system.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per haze per frame is the sizing this assumes, so the queue is as
// long as the table.
static const struct voe_ecs_key haze_intent_key = { "voe_scene_haze_intent" };

static const voe_base_struct_description *haze_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_haze_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_haze_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;
	const voe_scene_haze row = { .haze = 1000.0f, .start = 100.0f,
				     .height = 1000.0f };

	VOE_BASE_ASSERT(world != NULL, "registering hazes in no world");

	type = voe_ecs_component_register(world, &voe_scene_haze_key,
					  sizeof(voe_scene_haze), capacity,
					  haze_description());
	intent = voe_ecs_intent_register(world, &haze_intent_key,
					 sizeof(voe_scene_haze_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_haze_intent, haze));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_menu_set(world, type, "Rendering / Haze");
}

// haze and start finite and not negative, height finite; written so a NaN
// fails each.
static bool haze_valid(voe_scene_haze haze)
{
	return isfinite(haze.haze) && haze.haze >= 0.0f &&
	       isfinite(haze.start) && haze.start >= 0.0f &&
	       isfinite(haze.height);
}

bool voe_scene_haze_add(voe_ecs_world *world, voe_ecs_entity entity,
			voe_scene_haze haze)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a haze to no world");
	VOE_BASE_DEBUG_ASSERT(haze_valid(haze),
			      "a haze with a number not finite, or a haze or start below 0");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_haze_key),
		entity, &haze);
}

bool voe_scene_haze_submit(voe_ecs_world *world, voe_scene_haze_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a haze to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &haze_intent_key), &intent);
}

void voe_scene_haze_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_haze_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the haze system on no world");

	queue = voe_ecs_intent_type(world, &haze_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_haze_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!haze_valid(intents[i].haze)) {
			fprintf(stderr,
				"error: haze %uv%u: a number not finite, or a haze or start below 0, refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].haze);
	}

	voe_ecs_intent_clear(world, queue);
}
