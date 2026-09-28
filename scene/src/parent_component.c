// The parent component: its key, the read, and the two walks, up to an ancestor
// and down a tree. Everything that writes one is in parent_system.c.
//
// A WORLD MAY NEVER REGISTER THE TABLE, so the type is found by walking the
// world's types for the key's address rather than asking
// voe_ecs_component_type, which asserts on an unregistered key.
//
// THE TREE WALK SCANS THE WHOLE TABLE ONCE PER ENTITY IT VISITS, a deliberate
// ceiling: fine for scenes of hundreds of parented rows. A children index kept
// by the system would lift it.
#include <base/assert.h>
#include <ecs/component.h>
#include <scene/parent_component.h>
#include <scene/transform_component.h>

const struct voe_ecs_key voe_scene_parent_key = { "voe_scene_parent" };

static bool parent_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_scene_parent_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

static bool same(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

const voe_scene_parent *voe_scene_parent_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	voe_ecs_type type;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a parent out of no world");

	if (!parent_type(world, &type))
		return NULL;
	return voe_ecs_component_get(world, type, entity);
}

// The next link up: false when there is no row, or its parent is dead or has
// no transform, which ends the chain as if there were none.
static bool parent_link(const voe_ecs_world *world, voe_ecs_entity entity,
			voe_ecs_entity *out)
{
	const voe_scene_parent *row = voe_scene_parent_get(world, entity);

	if (row == NULL || voe_scene_transform_get(world, row->parent) == NULL)
		return false;
	*out = row->parent;
	return true;
}

bool voe_scene_parent_within(const voe_ecs_world *world, voe_ecs_entity entity,
			     voe_ecs_entity ancestor)
{
	voe_ecs_entity at = entity;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "walking parents in no world");

	for (uint32_t link = 0; link <= VOE_SCENE_PARENT_DEPTH_MAX; link++) {
		if (same(at, ancestor))
			return true;
		if (!parent_link(world, at, &at))
			return false;
	}
	return false;
}

// One entity on the walk down, and the parent row to look at next for its
// children.
struct frame {
	voe_ecs_entity entity;
	uint32_t next;
};

uint32_t voe_scene_parent_tree(const voe_ecs_world *world, voe_ecs_entity root,
			       voe_ecs_entity *out, uint32_t capacity)
{
	struct frame stack[VOE_SCENE_PARENT_DEPTH_MAX + 1];
	const voe_scene_parent *rows = NULL;
	const voe_ecs_entity *owners = NULL;
	voe_ecs_type type;
	uint32_t count = 0;
	uint32_t depth = 1;
	uint32_t written = 0;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "walking a tree in no world");
	VOE_BASE_DEBUG_ASSERT(out != NULL || capacity == 0,
			      "writing a tree to nowhere");

	if (capacity == 0)
		return 0;
	out[written++] = root;
	if (!parent_type(world, &type))
		return written;
	rows = voe_ecs_component_rows(world, type);
	owners = voe_ecs_component_entities(world, type);
	count = voe_ecs_component_count(world, type);
	stack[0] = (struct frame){ root, 0 };

	// Each pass writes one entity or pops one frame, so it is bounded by
	// twice the capacity.
	while (depth > 0 && written < capacity) {
		struct frame *top = &stack[depth - 1];
		bool expands = depth < VOE_SCENE_PARENT_DEPTH_MAX + 1 &&
			       voe_scene_transform_get(world, top->entity) != NULL;

		while (expands && top->next < count &&
		       !same(rows[top->next].parent, top->entity))
			top->next++;
		if (!expands || top->next == count) {
			depth--;
			continue;
		}
		out[written++] = owners[top->next];
		stack[depth++] = (struct frame){ owners[top->next++], 0 };
	}

	VOE_BASE_DEBUG_ASSERT(written <= capacity, "a tree past its capacity");
	return written;
}
