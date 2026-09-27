// The light system: registration, the one direct creation call, and the drain
// that applies a whole light or keeps the row over one it refuses.
//
// THE DRAIN IS THE WHOLE SYSTEM, as it is for transform. There is nothing per
// frame to compute: what a light does to a surface is the shader's arithmetic,
// and which way it shines is its transform's.
//
// AN INTENT FOR AN ENTITY WITH NO LIGHT IS DROPPED, silently: a destroyed entity
// and one that never had a light are both ordinary — see the header. A refused
// light is not, and says so once per intent on stderr, naming the entity by its
// index and generation as the camera's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per light per frame is the sizing this assumes, so the queue is as
// long as the table.
static const struct voe_ecs_key light_intent_key = { "voe_scene_light_intent" };

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
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering lights in no world");

	// Asserts when no transform was registered: a light is turned by one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_light_key,
					  sizeof(voe_scene_light), capacity,
					  light_description());
	intent = voe_ecs_intent_register(world, &light_intent_key,
					 sizeof(voe_scene_light_intent),
					 capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_scene_light_intent, light));
	voe_ecs_component_default_set(
		world, type,
		&(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				    .intensity = 1.0f,
				    .fill_colour = { 1.0f, 1.0f, 1.0f },
				    .fill_intensity = 0.0f });
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Rendering / Light");
}

// A linear channel: finite and within [0, 1]. Written so a NaN fails it.
static bool channel_valid(float value)
{
	return isfinite(value) && value >= 0.0f && value <= 1.0f;
}

static bool colour_valid(voe_math_float3 colour)
{
	return channel_valid(colour.x) && channel_valid(colour.y) &&
	       channel_valid(colour.z);
}

static bool intensity_valid(float value)
{
	return isfinite(value) && value >= 0.0f;
}

// The field that makes this light one the drain refuses, or NULL when none does.
static const char *refused_field(voe_scene_light light)
{
	if (!colour_valid(light.colour))
		return "colour";
	if (!intensity_valid(light.intensity))
		return "intensity";
	if (!colour_valid(light.fill_colour))
		return "fill_colour";
	if (!intensity_valid(light.fill_intensity))
		return "fill_intensity";
	return NULL;
}

bool voe_scene_light_add(voe_ecs_world *world, voe_ecs_entity entity,
			 voe_scene_light light)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a light to no world");
	VOE_BASE_DEBUG_ASSERT(refused_field(light) == NULL,
			      "a light with a colour outside [0, 1], a negative intensity or a number that is not finite");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_light_key),
		entity, &light);
}

bool voe_scene_light_submit(voe_ecs_world *world, voe_scene_light_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a light to no world");

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

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_entity entity = intents[i].entity;
		const char *field;

		if (voe_ecs_component_get(world, type, entity) == NULL)
			continue;
		field = refused_field(intents[i].light);
		if (field != NULL) {
			fprintf(stderr,
				"error: light %uv%u: %s refused, light kept\n",
				entity.index, entity.generation, field);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].light);
	}

	voe_ecs_intent_clear(world, queue);
}
