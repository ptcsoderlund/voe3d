// The mesh component: its key, its creation call, the one write after creation,
// and the reads the draw system does.
#include <3d/mesh_component.h>
#include <base/assert.h>

const struct voe_ecs_key voe_3d_mesh_key = { "voe_3d_mesh" };

void voe_3d_mesh_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering meshes in no world");

	(void)voe_ecs_component_register(world, &voe_3d_mesh_key,
					 sizeof(voe_3d_mesh), capacity);
}

bool voe_3d_mesh_add(voe_ecs_world *world, voe_ecs_entity entity,
		     voe_3d_mesh mesh)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a mesh to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_3d_mesh_key), entity,
		&mesh);
}

bool voe_3d_mesh_set_geometry(voe_ecs_world *world, voe_ecs_entity entity,
			      voe_render_geometry geometry)
{
	voe_ecs_type type;
	const voe_3d_mesh *current;
	voe_3d_mesh changed;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "changing a mesh in no world");

	type = voe_ecs_component_type(world, &voe_3d_mesh_key);
	current = voe_ecs_component_get(world, type, entity);
	if (current == NULL)
		return false;

	// Read, change one field, write the row back: the layer travels through
	// untouched, which is the whole of why this is not a _set taking a row.
	changed = *current;
	changed.geometry = geometry;
	return voe_ecs_component_set(world, type, entity, &changed);
}

const voe_3d_mesh *voe_3d_mesh_get(const voe_ecs_world *world,
				   voe_ecs_entity entity)
{
	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_3d_mesh_key), entity);
}

uint32_t voe_3d_mesh_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_3d_mesh_key));
}

const voe_3d_mesh *voe_3d_mesh_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_3d_mesh_key));
}

const voe_ecs_entity *voe_3d_mesh_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_3d_mesh_key));
}
