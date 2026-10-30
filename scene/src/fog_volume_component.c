// The fog volume component: its key, the drift's key and the reads.
// Everything that writes either table is in fog_volume_system.c, which is
// what makes a file including only this header provably a reader.
#include <base/assert.h>
#include <scene/fog_volume_component.h>

const struct voe_ecs_key voe_scene_fog_volume_key = { "voe_scene_fog_volume" };
const struct voe_ecs_key voe_scene_fog_volume_drift_key = {
	"voe_scene_fog_volume_drift"
};

const voe_scene_fog_volume *
voe_scene_fog_volume_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a fog volume out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_fog_volume_key),
		entity);
}

uint32_t voe_scene_fog_volume_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_fog_volume_key));
}

const voe_scene_fog_volume *
voe_scene_fog_volume_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_fog_volume_key));
}

const voe_ecs_entity *voe_scene_fog_volume_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_fog_volume_key));
}

const voe_scene_fog_volume_drift *
voe_scene_fog_volume_drift_get(const voe_ecs_world *world,
			       voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a fog drift out of no world");

	return voe_ecs_component_get(
		world,
		voe_ecs_component_type(world, &voe_scene_fog_volume_drift_key),
		entity);
}
