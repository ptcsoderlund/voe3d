// The picture system: registration, the one direct creation call, and the
// drain that applies a whole row or keeps the row over one it refuses.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for the sky. What exposure and tone
// do to the image is the renderer's.
//
// AN INTENT FOR AN ENTITY WITH NO PICTURE IS DROPPED, silently — see the
// header. A refused row says so once per intent on stderr, naming the entity by
// its index and generation as the sky's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/picture_system.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per picture per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key picture_intent_key = {
	"voe_scene_picture_intent"
};

static const voe_base_struct_description *picture_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_picture_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_picture_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;
	const voe_scene_picture row = { .exposure = 0.0f,
					.tone = VOE_SCENE_PICTURE_TONE_SOFT };

	VOE_BASE_ASSERT(world != NULL, "registering pictures in no world");

	type = voe_ecs_component_register(world, &voe_scene_picture_key,
					  sizeof(voe_scene_picture), capacity,
					  picture_description());
	intent = voe_ecs_intent_register(world, &picture_intent_key,
					 sizeof(voe_scene_picture_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_picture_intent,
					       picture));
	voe_ecs_component_default_set(world, type, &row);
	voe_ecs_component_menu_set(world, type, "Rendering / Picture");
}

// Every bound of the header at once; written so a NaN fails each comparison
// and an infinity is past the bound.
static bool picture_valid(voe_scene_picture picture)
{
	return isfinite(picture.exposure) &&
	       picture.exposure >= -VOE_SCENE_PICTURE_EXPOSURE_MAX &&
	       picture.exposure <= VOE_SCENE_PICTURE_EXPOSURE_MAX &&
	       picture.tone <= VOE_SCENE_PICTURE_TONE_SOFT;
}

bool voe_scene_picture_add(voe_ecs_world *world, voe_ecs_entity entity,
			   voe_scene_picture picture)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a picture to no world");
	VOE_BASE_DEBUG_ASSERT(picture_valid(picture),
			      "a picture with an exposure or tone out of bounds");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_picture_key),
		entity, &picture);
}

bool voe_scene_picture_submit(voe_ecs_world *world,
			      voe_scene_picture_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a picture to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &picture_intent_key),
		&intent);
}

void voe_scene_picture_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_picture_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the picture system on no world");

	queue = voe_ecs_intent_type(world, &picture_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_picture_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		if (!picture_valid(intents[i].picture)) {
			fprintf(stderr,
				"error: picture %uv%u: an exposure or tone out of bounds refused, row kept\n",
				entity.index, entity.generation);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].picture);
	}

	voe_ecs_intent_clear(world, queue);
}
