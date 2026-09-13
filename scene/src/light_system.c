// The light system: registration, the one direct creation call, the drain, and
// the normalization both writes go through.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for transform. There is nothing per
// frame to compute: what a light does to a surface is the shader's arithmetic,
// and where the light points is whatever was last submitted.
//
// AN INTENT FOR AN ENTITY WITH NO LIGHT IS DROPPED. voe_ecs_component_set
// returns false for a destroyed entity and for one that never had a light, and
// both are ordinary rather than wrong — see the header.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <math/float3.h>
#include <scene/light_system.h>

// One intent per light per frame is the sizing this assumes, so the queue is as
// long as the table.
static const struct voe_ecs_key light_intent_key = { "voe_scene_light_intent" };

// The one place a direction becomes unit length. Called by both writes, which is
// what lets light_component.h promise a reader that it already is.
static voe_scene_light settled(voe_scene_light light)
{
	VOE_BASE_ASSERT(voe_math_float3_length(light.direction) > 0.0f,
			"a light with no direction — see scene/light_component.h on which way a direction points");

	light.direction = voe_math_float3_normalize(light.direction);
	return light;
}

static const voe_base_struct_description *light_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_light_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_light_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering lights in no world");

	(void)voe_ecs_component_register(world, &voe_scene_light_key,
					 sizeof(voe_scene_light), capacity,
					 light_description());
	(void)voe_ecs_intent_register(world, &light_intent_key,
				      sizeof(voe_scene_light_intent),
				      capacity);
}

bool voe_scene_light_add(voe_ecs_world *world, voe_ecs_entity entity,
			 voe_scene_light light)
{
	voe_scene_light unit = settled(light);

	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a light to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_light_key),
		entity, &unit);
}

bool voe_scene_light_submit(voe_ecs_world *world, voe_scene_light_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a light to no world");

	// Normalized on the way into the queue rather than on the way out, so
	// that a submitter with a direction of nothing hears about it at the
	// call that made the mistake and not a frame later in the drain.
	intent.light = settled(intent.light);

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &light_intent_key), &intent);
}

void voe_scene_light_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_light_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the light system on no world");

	queue = voe_ecs_intent_type(world, &light_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_light_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++)
		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &intents[i].light);

	voe_ecs_intent_clear(world, queue);
}
