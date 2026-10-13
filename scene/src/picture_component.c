// The picture component: its key, its tone names and the reads. Everything
// that writes one is in picture_system.c, which is what makes a file including
// only this header provably a reader.
#include <base/assert.h>
#include <scene/picture_component.h>

const struct voe_ecs_key voe_scene_picture_key = { "voe_scene_picture" };

const char *const voe_scene_picture_tone_names[VOE_SCENE_PICTURE_TONE_SOFT +
					       1] = { "Clip", "Soft" };

const voe_scene_picture *voe_scene_picture_get(const voe_ecs_world *world,
					       voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a picture out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_picture_key),
		entity);
}

uint32_t voe_scene_picture_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_picture_key));
}

const voe_scene_picture *voe_scene_picture_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_picture_key));
}

const voe_ecs_entity *voe_scene_picture_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_picture_key));
}
