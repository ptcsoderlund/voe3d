// The shape component: its key, the registration and the reads. Everything
// that writes one is in shape_system.c, which is what makes a file including
// only this header provably a reader.
#include <3d/shape_component.h>
#include <base/assert.h>

const struct voe_ecs_key voe_3d_shape_key = { "voe_3d_shape" };

static const voe_base_struct_description *shape_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_3d_shape_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_3d_shape_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering shapes in no world");

	(void)voe_ecs_component_register(world, &voe_3d_shape_key,
					 sizeof(voe_3d_shape), capacity,
					 shape_description());
}

bool voe_3d_shape_add(voe_ecs_world *world, voe_ecs_entity entity,
		      voe_3d_shape shape)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a shape to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_3d_shape_key), entity,
		&shape);
}

const voe_3d_shape *voe_3d_shape_get(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_3d_shape_key), entity);
}

uint32_t voe_3d_shape_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_3d_shape_key));
}

const voe_3d_shape *voe_3d_shape_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_3d_shape_key));
}

const voe_ecs_entity *voe_3d_shape_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_3d_shape_key));
}
