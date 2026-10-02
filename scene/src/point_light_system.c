// The point light system: registration, the one direct creation call, and the
// run that applies whole lights in submission order.
//
// A refused light says so once per intent on stderr, naming the entity by its
// index and generation as the sun's drain does.
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/intent.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per light per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key replace_key = { "voe_scene_point_light_intent" };

static const voe_base_struct_description *point_light_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_point_light_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_point_light_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_type transform;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering point lights in no world");

	// Asserts when no transform was registered: a light is placed by one.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_point_light_key,
					  sizeof(voe_scene_point_light),
					  capacity, point_light_description());
	intent = voe_ecs_intent_register(world, &replace_key,
					 sizeof(voe_scene_point_light_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_scene_point_light_intent, light));
	voe_ecs_component_default_set(
		world, type,
		&(voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					  .intensity = 1.0f,
					  .range = 5.0f,
					  .falloff = 1.0f,
					  .cast_shadows = false,
					  .bounces = 0,
					  .bounce_strength = 1.0f });
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Rendering / Point light");
}

// A linear channel: finite and within [0, 1]. Written so a NaN fails it.
static bool channel_valid(float value)
{
	return isfinite(value) && value >= 0.0f && value <= 1.0f;
}

// The field that makes this light one the drain refuses, or NULL when none does.
// Each comparison is written so a NaN fails it.
static const char *refused_field(voe_scene_point_light light)
{
	if (!channel_valid(light.colour.x) || !channel_valid(light.colour.y) ||
	    !channel_valid(light.colour.z))
		return "colour";
	if (!isfinite(light.intensity) || light.intensity < 0.0f)
		return "intensity";
	if (!isfinite(light.range) || light.range <= 0.0f)
		return "range";
	if (!(light.falloff >= VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST &&
	      light.falloff <= VOE_SCENE_POINT_LIGHT_FALLOFF_MOST))
		return "falloff";
	if (light.bounces > VOE_SCENE_LIGHT_BOUNCES_MAX)
		return "bounces";
	if (!isfinite(light.bounce_strength) || light.bounce_strength < 0.0f)
		return "bounce_strength";
	return NULL;
}

bool voe_scene_point_light_add(voe_ecs_world *world, voe_ecs_entity entity,
			       voe_scene_point_light light)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a point light to no world");
	VOE_BASE_DEBUG_ASSERT(refused_field(light) == NULL,
			      "a point light with a colour outside [0, 1], a negative intensity, a range of nought or less, a falloff outside its bounds, too many bounces, a negative bounce strength or a number that is not finite");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key),
		entity, &light);
}

bool voe_scene_point_light_submit(voe_ecs_world *world,
				  voe_scene_point_light_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "submitting a point light to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &replace_key), &intent);
}

void voe_scene_point_light_system_run(voe_ecs_world *world)
{
	voe_ecs_type type;
	voe_ecs_intent queue;
	const voe_scene_point_light_intent *intents;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the point light system on no world");

	type = voe_ecs_component_type(world, &voe_scene_point_light_key);
	queue = voe_ecs_intent_type(world, &replace_key);
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
				"error: point light %uv%u: %s refused, light kept\n",
				entity.index, entity.generation, field);
			continue;
		}
		(void)voe_ecs_component_set(world, type, entity,
					    &intents[i].light);
	}
	voe_ecs_intent_clear(world, queue);
}
