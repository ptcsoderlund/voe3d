// The point light system: registration, the one direct creation call, and the
// run that applies whole lights, keeps glow rows one per light and fades them.
//
// THE COUNTDOWN COMES FIRST IN THE CODE, and that is what keeps a flash given
// this run from being counted down this run: every glow there was is faded,
// then the replaces, the new rows and the flashes write full ones over it. The
// outcome is the order the header gives.
//
// A GLOW ROW WHOSE LIGHT IS GONE IS DROPPED before any is made, walking back so
// a row swapped into a hole has been seen, as the emitter's particles are. So
// the glow table never holds more rows than the light table, and with the same
// capacity adding one cannot fail.
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

// One intent per light per frame is the sizing this assumes, so each queue is
// as long as the table.
static const struct voe_ecs_key replace_key = { "voe_scene_point_light_intent" };
static const struct voe_ecs_key flash_key = { "voe_scene_point_light_flash" };

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
					  .flash = 0.0f,
					  .flash_when_made = false });
	voe_ecs_component_needs_set(world, type, transform);
	voe_ecs_component_menu_set(world, type, "Rendering / Point light");

	(void)voe_ecs_intent_register(world, &flash_key, sizeof(voe_ecs_entity),
				      capacity);
	(void)voe_ecs_component_register(world, &voe_scene_point_light_glow_key,
					 sizeof(voe_scene_point_light_glow),
					 capacity, &voe_ecs_runtime_only);
}

// A linear channel: finite and within [0, 1]. Written so a NaN fails it.
static bool channel_valid(float value)
{
	return isfinite(value) && value >= 0.0f && value <= 1.0f;
}

// The field that makes this light one the drain refuses, or NULL when none does.
static const char *refused_field(voe_scene_point_light light)
{
	if (!channel_valid(light.colour.x) || !channel_valid(light.colour.y) ||
	    !channel_valid(light.colour.z))
		return "colour";
	if (!isfinite(light.intensity) || light.intensity < 0.0f)
		return "intensity";
	if (!isfinite(light.range) || light.range <= 0.0f)
		return "range";
	if (!isfinite(light.flash) || light.flash < 0.0f)
		return "flash";
	return NULL;
}

bool voe_scene_point_light_add(voe_ecs_world *world, voe_ecs_entity entity,
			       voe_scene_point_light light)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a point light to no world");
	VOE_BASE_DEBUG_ASSERT(refused_field(light) == NULL,
			      "a point light with a colour outside [0, 1], a negative intensity or flash, a range of nought or less or a number that is not finite");

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

bool voe_scene_point_light_flash_submit(voe_ecs_world *world,
					voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "flashing a point light in no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &flash_key), &entity);
}

// The entity's glow, set to `left` or made with it. Made only for a live light,
// so neither a dead entity nor a full table can refuse it — see the header.
static void glow_put(voe_ecs_world *world, voe_ecs_type glows,
		     voe_ecs_entity entity, float left)
{
	const voe_scene_point_light_glow glow = { .left = left };

	if (!voe_ecs_component_set(world, glows, entity, &glow))
		(void)voe_ecs_component_add(world, glows, entity, &glow);
}

// Drops the glows whose light is gone, then fades the rest by `seconds`.
static void drop_and_fade(voe_ecs_world *world, voe_ecs_type glows,
			  float seconds)
{
	const voe_ecs_entity *owners = voe_ecs_component_entities(world, glows);
	const voe_scene_point_light_glow *rows;

	for (uint32_t i = voe_ecs_component_count(world, glows); i > 0; i--)
		if (voe_scene_point_light_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, glows,
						       owners[i - 1]);

	rows = voe_ecs_component_rows(world, glows);
	for (uint32_t i = 0; i < voe_ecs_component_count(world, glows); i++)
		glow_put(world, glows, owners[i],
			 fmaxf(rows[i].left - seconds, 0.0f));
}

// Applies the waiting replaces in order, restarting a flashing light's flash.
static void apply_replaces(voe_ecs_world *world, voe_ecs_type type,
			   voe_ecs_type glows)
{
	voe_ecs_intent queue = voe_ecs_intent_type(world, &replace_key);
	const voe_scene_point_light_intent *intents =
		voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

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
		if (intents[i].light.flash > 0.0f)
			glow_put(world, glows, entity, intents[i].light.flash);
	}
	voe_ecs_intent_clear(world, queue);
}

// Gives each light with no glow its first: full when it flashes when made.
static void make_glows(voe_ecs_world *world, voe_ecs_type glows)
{
	const voe_scene_point_light *lights = voe_scene_point_light_rows(world);
	const voe_ecs_entity *entities = voe_scene_point_light_entities(world);

	for (uint32_t i = 0; i < voe_scene_point_light_count(world); i++) {
		bool full = lights[i].flash > 0.0f && lights[i].flash_when_made;

		if (voe_ecs_component_get(world, glows, entities[i]) == NULL)
			glow_put(world, glows, entities[i],
				 full ? lights[i].flash : 0.0f);
	}
}

// Applies the waiting flashes; one for no light or a steady one is ignored.
static void apply_flashes(voe_ecs_world *world, voe_ecs_type glows)
{
	voe_ecs_intent queue = voe_ecs_intent_type(world, &flash_key);
	const voe_ecs_entity *entities = voe_ecs_intent_queue(world, queue);
	uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_point_light *light =
			voe_scene_point_light_get(world, entities[i]);

		if (light != NULL && light->flash > 0.0f)
			glow_put(world, glows, entities[i], light->flash);
	}
	voe_ecs_intent_clear(world, queue);
}

void voe_scene_point_light_system_run(voe_ecs_world *world, float seconds)
{
	voe_ecs_type type;
	voe_ecs_type glows;

	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "running the point light system on no world");
	VOE_BASE_ASSERT(isfinite(seconds) && seconds >= 0.0f,
			"running point lights by a time that is negative or not finite");

	type = voe_ecs_component_type(world, &voe_scene_point_light_key);
	glows = voe_ecs_component_type(world, &voe_scene_point_light_glow_key);

	drop_and_fade(world, glows, seconds);
	apply_replaces(world, type, glows);
	make_glows(world, glows);
	apply_flashes(world, glows);
}
