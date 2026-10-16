// The identity component: its key, and the reads anyone may do.
//
// NOTHING IN THIS FILE WRITES. Every write to this table is in
// identity_system.c, which is the whole of what "read by anyone, written by
// one" costs to arrange.
//
// THE SWITCH READ MAY MEET A WORLD THAT NEVER REGISTERED THE TABLE, so it finds
// the type by walking the world's types for the key's address, as
// parent_component.c does, rather than asking voe_ecs_component_type, which
// asserts on an unregistered key.
#include <base/assert.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>

const struct voe_ecs_key voe_scene_identity_key = { "voe_scene_identity" };

static bool identity_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_scene_identity_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

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

bool voe_scene_identity_off(const voe_ecs_world *world, voe_ecs_entity entity)
{
	voe_ecs_entity at = entity;
	voe_ecs_type type;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a switch out of no world");

	if (!identity_type(world, &type))
		return false;
	for (uint32_t link = 0; link <= VOE_SCENE_PARENT_DEPTH_MAX; link++) {
		const voe_scene_identity *row =
			voe_ecs_component_get(world, type, at);
		const voe_scene_parent *parent = voe_scene_parent_get(world, at);

		if (row != NULL && row->off)
			return true;
		if (parent == NULL)
			return false;
		at = parent->parent;
	}
	return false;
}
