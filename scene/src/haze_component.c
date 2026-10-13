// The haze component: its key and the reads. Everything that writes one is in
// haze_system.c, which is what makes a file including only this header
// provably a reader.
#include <base/assert.h>
#include <scene/haze_component.h>

const struct voe_ecs_key voe_scene_haze_key = { "voe_scene_haze" };

const voe_scene_haze *voe_scene_haze_get(const voe_ecs_world *world,
					 voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a haze out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_haze_key),
		entity);
}

uint32_t voe_scene_haze_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_haze_key));
}

const voe_scene_haze *voe_scene_haze_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_haze_key));
}

const voe_ecs_entity *voe_scene_haze_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_haze_key));
}
