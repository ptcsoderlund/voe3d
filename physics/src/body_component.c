// The body component: its key and the reads. Everything that writes one is in
// body_system.c, which is what makes a file including only the component
// header provably a reader.
#include <base/assert.h>
#include <physics/body_component.h>

#include <stddef.h>

const struct voe_ecs_key voe_physics_body_key = { "voe_physics_body" };

const voe_physics_body *voe_physics_body_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a body out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_physics_body_key),
		entity);
}

uint32_t voe_physics_body_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_physics_body_key));
}

const voe_physics_body *voe_physics_body_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_physics_body_key));
}

const voe_ecs_entity *voe_physics_body_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_physics_body_key));
}
