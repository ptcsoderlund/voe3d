// The panel component: its key, its creation call, the one write that happens
// every frame, and the reads the draw system does.
#include <3d/panel_component.h>
#include <base/assert.h>

const struct voe_ecs_key voe_3d_panel_key = { "voe_3d_panel" };

void voe_3d_panel_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering panels in no world");

	(void)voe_ecs_component_register(world, &voe_3d_panel_key,
					 sizeof(voe_3d_panel), capacity, NULL);
}

bool voe_3d_panel_add(voe_ecs_world *world, voe_ecs_entity entity,
		      voe_3d_panel panel)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a panel to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_3d_panel_key), entity,
		&panel);
}

bool voe_3d_panel_set_range(voe_ecs_world *world, voe_ecs_entity entity,
			    uint32_t first, uint32_t count)
{
	voe_ecs_type type;
	const voe_3d_panel *current;
	voe_3d_panel changed;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "changing a panel in no world");

	type = voe_ecs_component_type(world, &voe_3d_panel_key);
	current = voe_ecs_component_get(world, type, entity);
	if (current == NULL)
		return false;

	// Read, change the two numbers, write the row back: the size and the
	// layer travel through untouched, which is the whole of why this is not
	// a _set taking a row.
	changed = *current;
	changed.first = first;
	changed.count = count;
	return voe_ecs_component_set(world, type, entity, &changed);
}

const voe_3d_panel *voe_3d_panel_get(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_3d_panel_key), entity);
}

uint32_t voe_3d_panel_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_3d_panel_key));
}

const voe_3d_panel *voe_3d_panel_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_3d_panel_key));
}

const voe_ecs_entity *voe_3d_panel_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_3d_panel_key));
}
