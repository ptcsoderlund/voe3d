// The parent system: registration. There is no drain and no intent; rows are
// added and removed through the world's structural queue (0190, 0281).
#include <base/assert.h>
#include <ecs/component.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>

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
	voe_ecs_type transform;

	VOE_BASE_ASSERT(world != NULL, "registering parents in no world");

	// Asserts when no transform was registered: a parent places a transform.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	type = voe_ecs_component_register(world, &voe_scene_parent_key,
					  sizeof(voe_scene_parent), capacity,
					  parent_description());
	voe_ecs_component_default_set(world, type, &(voe_scene_parent){ 0 });
	voe_ecs_component_needs_set(world, type, transform);

	VOE_BASE_DEBUG_ASSERT(voe_ecs_component_menu(world, type) == NULL,
			      "a parent offered in Add component");
}
