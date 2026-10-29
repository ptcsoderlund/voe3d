// The clouds component: its key, its kind names, the drift's key and the
// reads. Everything that writes either table is in clouds_system.c, which is
// what makes a file including only this header provably a reader.
#include <base/assert.h>
#include <scene/clouds_component.h>

const struct voe_ecs_key voe_scene_clouds_key = { "voe_scene_clouds" };
const struct voe_ecs_key voe_scene_clouds_drift_key = {
	"voe_scene_clouds_drift"
};

const char *const voe_scene_clouds_kind_names[VOE_SCENE_CLOUDS_VOLUMETRIC +
					      1] = { "Flat", "Volumetric" };

const voe_scene_clouds *voe_scene_clouds_get(const voe_ecs_world *world,
					     voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading clouds out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key),
		entity);
}

uint32_t voe_scene_clouds_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key));
}

const voe_scene_clouds *voe_scene_clouds_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key));
}

const voe_ecs_entity *voe_scene_clouds_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key));
}

const voe_scene_clouds_drift *
voe_scene_clouds_drift_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a drift out of no world");

	return voe_ecs_component_get(
		world,
		voe_ecs_component_type(world, &voe_scene_clouds_drift_key),
		entity);
}
