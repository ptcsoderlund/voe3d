// The point light component: its two keys, the reads, and the strength a reader
// draws with. Everything that writes either table is in point_light_system.c,
// which is what makes a file including only this header provably a reader.
#include <base/assert.h>
#include <scene/point_light_component.h>

const struct voe_ecs_key voe_scene_point_light_key = { "voe_scene_point_light" };
const struct voe_ecs_key voe_scene_point_light_glow_key = {
	"voe_scene_point_light_glow"
};

const voe_scene_point_light *
voe_scene_point_light_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "reading a point light out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key),
		entity);
}

uint32_t voe_scene_point_light_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}

const voe_scene_point_light *
voe_scene_point_light_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}

const voe_ecs_entity *voe_scene_point_light_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_point_light_key));
}

float voe_scene_point_light_strength(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	const voe_scene_point_light *light =
		voe_scene_point_light_get(world, entity);
	const voe_scene_point_light_glow *glow;

	if (light == NULL)
		return 0.0f;
	if (!(light->flash > 0.0f))
		return light->intensity;
	glow = voe_ecs_component_get(
		world,
		voe_ecs_component_type(world, &voe_scene_point_light_glow_key),
		entity);
	if (glow == NULL || !(glow->left > 0.0f))
		return 0.0f;
	return light->intensity * glow->left / light->flash;
}
