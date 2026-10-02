// The point light component: its key and the reads. Everything that writes the
// table is in point_light_system.c, which is what makes a file including only
// this header provably a reader.
#include <base/assert.h>
#include <scene/point_light_component.h>

const struct voe_ecs_key voe_scene_point_light_key = { "voe_scene_point_light" };

const voe_scene_point_light *
voe_scene_point_light_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "reading a point light out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key),
		entity);
}

uint32_t voe_scene_point_light_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}

const voe_scene_point_light *
voe_scene_point_light_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}

const voe_ecs_entity *voe_scene_point_light_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}
