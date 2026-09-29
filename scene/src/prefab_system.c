// The prefab system: registration of the prefab and prefab part tables and
// nothing else. There is no drain and no intent; rows are added through the
// world's structural queue or by a load (0283 point 2).
#include <base/assert.h>
#include <ecs/component.h>
#include <scene/prefab_system.h>
#include <scene/transform_component.h>

static const voe_base_struct_description *prefab_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_prefab_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

void voe_scene_prefab_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type prefab;
	voe_ecs_type part;
	voe_ecs_type transform;

	VOE_BASE_ASSERT(world != NULL, "registering prefabs in no world");

	// Asserts when no transform was registered: both rows place a transform.
	transform = voe_ecs_component_type(world, &voe_scene_transform_key);
	prefab = voe_ecs_component_register(world, &voe_scene_prefab_key,
					    sizeof(voe_scene_prefab), capacity,
					    prefab_description());
	voe_ecs_component_default_set(world, prefab, &(voe_scene_prefab){ 0 });
	voe_ecs_component_needs_set(world, prefab, transform);

	part = voe_ecs_component_register(world, &voe_scene_prefab_part_key,
					  sizeof(voe_scene_prefab_part),
					  capacity, &voe_ecs_runtime_only);
	voe_ecs_component_needs_set(world, part, transform);

	VOE_BASE_DEBUG_ASSERT(voe_ecs_component_menu(world, prefab) == NULL,
			      "a prefab offered in Add component");
	VOE_BASE_DEBUG_ASSERT(voe_ecs_component_runtime_only(world, part),
			      "a prefab part that would be saved");
}
