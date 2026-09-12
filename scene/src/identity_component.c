// The identity component: its key, and the reads anyone may do.
//
// NOTHING IN THIS FILE WRITES. Every write to this table is in
// identity_system.c, which is the whole of what "read by anyone, written by
// one" costs to arrange.
#include <base/assert.h>
#include <scene/identity_component.h>

const struct voe_ecs_key voe_scene_identity_key = { "voe_scene_identity" };

const voe_scene_identity *voe_scene_identity_get(const voe_ecs_world *world,
						 voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading an identity out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_identity_key),
		entity);
}

uint32_t voe_scene_identity_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_identity_key));
}

const voe_scene_identity *voe_scene_identity_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_identity_key));
}

const voe_ecs_entity *voe_scene_identity_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_identity_key));
}
