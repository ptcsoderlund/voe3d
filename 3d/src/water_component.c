// The water and waves components: their keys, the registration, the reads and
// the replace submit. The drain and the clock are the water system's; see
// 3d/water_component.h.
//
// THE REPLACE INTENT'S KEY IS THIS FILE'S ALONE: the submit and the system find
// that queue as the water's replace (ecs/component.h).
#include <3d/water_component.h>

#include <base/assert.h>

#include <ecs/intent.h>

#include <scene/transform_component.h>

#include <stddef.h>

const struct voe_ecs_key voe_3d_water_key = { "voe_3d_water" };
const struct voe_ecs_key voe_3d_waves_key = { "voe_3d_waves" };

static const struct voe_ecs_key water_intent_key = { "voe_3d_water_intent" };

// 0305 point 5's defaults; both colours linear.
static const voe_3d_water water_default = {
	.width = 20.0f,
	.length = 20.0f,
	.wave_height = 0.05f,
	.wave_length = 2.0f,
	.deep = 3.0f,
	.colour = { 0.02f, 0.09f, 0.10f },
	.sky = { 0.55f, 0.70f, 0.85f },
};

static const voe_base_struct_description *water_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_3d_water_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

static voe_ecs_type water_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_water_key);
}

static voe_ecs_type waves_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_waves_key);
}

void voe_3d_water_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering water in no world");

	type = voe_ecs_component_register(world, &voe_3d_water_key,
					  sizeof(voe_3d_water), capacity,
					  water_description());
	intent = voe_ecs_intent_register(world, &water_intent_key,
					 sizeof(voe_3d_water_intent), capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_3d_water_intent, water));
	voe_ecs_component_default_set(world, type, &water_default);
	voe_ecs_component_needs_set(
		world, type,
		voe_ecs_component_type(world, &voe_scene_transform_key));
	voe_ecs_component_menu_set(world, type, "Rendering / Water");

	(void)voe_ecs_component_register(world, &voe_3d_waves_key,
					 sizeof(voe_3d_waves), capacity,
					 &voe_ecs_runtime_only);
}

bool voe_3d_water_add(voe_ecs_world *world, voe_ecs_entity entity,
		      voe_3d_water water)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding water to no world");

	return voe_ecs_component_add(world, water_type(world), entity, &water);
}

const voe_3d_water *voe_3d_water_get(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, water_type(world), entity);
}

uint32_t voe_3d_water_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, water_type(world));
}

const voe_3d_water *voe_3d_water_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, water_type(world));
}

const voe_ecs_entity *voe_3d_water_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, water_type(world));
}

const voe_3d_waves *voe_3d_waves_get(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, waves_type(world), entity);
}

uint32_t voe_3d_waves_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, waves_type(world));
}

const voe_3d_waves *voe_3d_waves_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, waves_type(world));
}

const voe_ecs_entity *voe_3d_waves_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, waves_type(world));
}

bool voe_3d_water_submit(voe_ecs_world *world, voe_3d_water_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting water to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_component_replace(world, water_type(world)).intent,
		&intent);
}
