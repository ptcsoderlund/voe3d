// The light blocker component: its key, its Block names and the reads.
// Everything that writes one is in light_blocker_system.c, which is what makes
// a file including only this header provably a reader.
#include <base/assert.h>
#include <scene/light_blocker_component.h>

const struct voe_ecs_key voe_scene_light_blocker_key = {
	"voe_scene_light_blocker"
};

const char *const voe_scene_light_blocker_block_names[3] = { "All", "Fill",
							     "Direct" };

const voe_scene_light_blocker *
voe_scene_light_blocker_get(const voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL,
			      "reading a light blocker out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_light_blocker_key),
		entity);
}

uint32_t voe_scene_light_blocker_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_light_blocker_key));
}

const voe_scene_light_blocker *
voe_scene_light_blocker_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_light_blocker_key));
}

const voe_ecs_entity *
voe_scene_light_blocker_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_light_blocker_key));
}
