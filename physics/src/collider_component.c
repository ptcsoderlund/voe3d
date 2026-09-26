// The collider component: its key, its kind names and the reads. Everything
// that writes one is in collider_system.c, which is what makes a file including
// only the component header provably a reader.
#include <base/assert.h>
#include <physics/collider_component.h>

#include <stddef.h>

const struct voe_ecs_key voe_physics_collider_key = { "voe_physics_collider" };

// In the kinds' own order: the index is the kind, so nought is no kind.
const char *const voe_physics_collider_kind_names[4] = { NULL, "Box", "Sphere",
							 "Capsule" };

const voe_physics_collider *
voe_physics_collider_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a collider out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_physics_collider_key),
		entity);
}

uint32_t voe_physics_collider_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_physics_collider_key));
}

const voe_physics_collider *voe_physics_collider_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_physics_collider_key));
}

const voe_ecs_entity *voe_physics_collider_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_physics_collider_key));
}
