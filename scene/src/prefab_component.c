// The prefab and prefab part components: their keys and the two reads.
// Everything that registers them is in prefab_system.c.
//
// A WORLD MAY NEVER REGISTER EITHER TABLE, so a type is found by walking the
// world's types for the key's address rather than asking
// voe_ecs_component_type, which asserts on an unregistered key.
#include <base/assert.h>
#include <ecs/component.h>
#include <scene/prefab_component.h>

const struct voe_ecs_key voe_scene_prefab_key = { "voe_scene_prefab" };
const struct voe_ecs_key voe_scene_prefab_part_key = {
	"voe_scene_prefab_part"
};

// The row of `entity` in the table registered against `key`, or NULL.
static const void *row_of(const voe_ecs_world *world,
			  const struct voe_ecs_key *key, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a prefab out of no world");
	VOE_BASE_DEBUG_ASSERT(key != NULL, "reading a prefab under no key");

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == key)
			return voe_ecs_component_get(world, type, entity);
	}
	return NULL;
}

const voe_scene_prefab *voe_scene_prefab_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	return row_of(world, &voe_scene_prefab_key, entity);
}

const voe_scene_prefab_part *
voe_scene_prefab_part_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	return row_of(world, &voe_scene_prefab_part_key, entity);
}
