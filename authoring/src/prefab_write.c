// The prefab writer: refuses a root that cannot head a prefab, gathers its tree
// and hands it to scene_write.c's walk (scene_tree.h), which writes it.
//
// THE TREE IS GATHERED INTO THE ARENA, as long as the world holds entities, since
// no tree can hold more.
//
// THE IDENTITY TYPE IS FOUND BY WALKING THE TYPES, as scene_write.c does, because
// voe_scene_identity_get asserts on a world that registered none, and a root in
// such a world is only a root with no identity.
#include <authoring/prefab.h>

#include "scene_tree.h"

#include <base/assert.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>

#define MODULE "authoring"

static bool has_identity(const voe_ecs_world *world, voe_ecs_entity entity)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_scene_identity_key)
			return voe_ecs_component_get(world, type, entity) != NULL;
	}
	return false;
}

bool voe_authoring_prefab_write(const voe_ecs_world *world,
				voe_ecs_entity root, voe_base_arena *arena,
				voe_authoring_text *out)
{
	VOE_BASE_ASSERT(world != NULL, "writing a prefab out of a NULL world");
	VOE_BASE_ASSERT(arena != NULL, "writing a prefab into a NULL arena");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the prefab's text");

	if (!voe_ecs_entity_alive(world, root)) {
		VOE_BASE_ERROR(MODULE,
			       "entity %uv%u is not alive, so it heads no "
			       "prefab; nothing was written",
			       root.index, root.generation);
		return false;
	}
	if (!has_identity(world, root)) {
		VOE_BASE_ERROR(MODULE,
			       "entity %uv%u has no identity, and a prefab's "
			       "root is written under its id; nothing was "
			       "written",
			       root.index, root.generation);
		return false;
	}

	uint32_t capacity = voe_ecs_entity_count(world);
	voe_ecs_entity *entities =
		voe_base_arena_push(arena, capacity * sizeof(*entities));
	const voe_authoring_tree tree = {
		.root = root,
		.entities = entities,
		.count = voe_scene_parent_tree(world, root, entities, capacity),
	};

	VOE_BASE_ASSERT(tree.count >= 1, "a live root's tree holds the root");
	return voe_authoring_scene_write_tree(world, NULL, &tree, arena, out);
}
