// The sky component: its key and the reads. Everything that
// writes one is in sky_system.c, which is what makes a file including only
// this header provably a reader.
#include <base/assert.h>
#include <scene/sky_component.h>

const struct voe_ecs_key voe_scene_sky_key = { "voe_scene_sky" };

const voe_scene_sky *voe_scene_sky_get(const voe_ecs_world *world,
				       voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a sky out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_sky_key),
		entity);
}

uint32_t voe_scene_sky_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_sky_key));
}

const voe_scene_sky *voe_scene_sky_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_sky_key));
}

const voe_ecs_entity *voe_scene_sky_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_sky_key));
}
