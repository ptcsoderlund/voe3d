// The parent system: registration, and parenting as structural requests plus
// one transform intent. There is no drain and no intent of its own; rows are
// added and removed through the world's structural queue (0190, 0281).
#include <base/assert.h>
#include <ecs/component.h>
#include <ecs/structure.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

static const voe_base_struct_description *parent_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_parent_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_parent_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;

	VOE_BASE_ASSERT(world != NULL, "registering parents in no world");

	// Asserts when no transform was registered: _set reads the table. A parent
	// row itself needs none (0300), so no need is named.
	(void)voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_parent_key,
					  sizeof(voe_scene_parent), capacity,
					  parent_description());
	voe_ecs_component_default_set(world, type, &(voe_scene_parent){ 0 });

	VOE_BASE_DEBUG_ASSERT(voe_ecs_component_menu(world, type) == NULL,
			      "a parent offered in Add component");
}

bool voe_scene_parent_set(voe_ecs_world *world, voe_ecs_entity child,
			  voe_ecs_entity parent)
{
	bool root = parent.index == 0 && parent.generation == 0;
	voe_ecs_type type;
	voe_scene_transform placed;

	VOE_BASE_ASSERT(world != NULL, "parenting in no world");
	// Asserts when the world never registered the parent table.
	type = voe_ecs_component_type(world, &voe_scene_parent_key);
	VOE_BASE_ASSERT(root || voe_ecs_entity_alive(world, parent),
			"parenting under a dead thing");
	VOE_BASE_ASSERT(root || !voe_scene_parent_within(world, parent, child),
			"parenting a thing under itself or its own tree");

	if (!voe_ecs_structure_remove(world, type, child))
		return false;
	if (!root && !voe_ecs_structure_add(world, type, child,
					    &(voe_scene_parent){ parent }))
		return false;
	if (voe_scene_transform_get(world, child) == NULL)
		return true;

	// A parent without a transform ends the chain, so under one the child's
	// row is its world place as it is now.
	placed = voe_scene_transform_world(world, child);
	if (!root && voe_scene_transform_get(world, parent) != NULL)
		placed = voe_scene_transform_relative(
			voe_scene_transform_world(world, parent), placed);
	return voe_scene_transform_submit(world, (voe_scene_transform_intent){
		.entity = child, .transform = placed });
}
