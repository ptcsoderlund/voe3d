// The shape component: its key, the registration and the reads. Everything
// that writes one is in shape_system.c, which is what makes a file including
// only this header provably a reader.
//
// THE INTENT'S KEY IS THIS FILE'S ALONE. The drain and the submit find the queue
// as the shape's replace (ecs/component.h), so nothing else needs its name.
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/assert.h>
#include <scene/transform_component.h>

#include <stddef.h>

const struct voe_ecs_key voe_3d_shape_key = { "voe_3d_shape" };

// In the kinds' own order: the index is the kind, so nought is no kind.
const char *const voe_3d_shape_kind_names[4] = { NULL, "Cube", "Capsule",
						 "Cylinder" };

static const struct voe_ecs_key shape_intent_key = { "voe_3d_shape_intent" };

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
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering shapes in no world");

	type = voe_ecs_component_register(world, &voe_3d_shape_key,
					  sizeof(voe_3d_shape), capacity,
					  shape_description());
	intent = voe_ecs_intent_register(world, &shape_intent_key,
					 sizeof(voe_3d_shape_intent), capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_3d_shape_intent, shape));
	voe_ecs_component_default_set(
		world, type,
		&(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				 .colour = VOE_3D_SHAPE_GREY });
	voe_ecs_component_needs_set(
		world, type,
		voe_ecs_component_type(world, &voe_scene_transform_key));
	voe_ecs_component_menu_set(world, type, "Rendering / Shape");
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
