// A fresh game world has every one of the eight types registered: each key
// resolves through voe_ecs_component_type, which asserts on a key nothing
// registered, and the eight answers are eight different types. Needs no
// window and no graphics card.
#include <game/world.h>

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>

#include <base/arena.h>

#include <ecs/component.h>

#include <scene/camera_component.h>
#include <scene/identity_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#define TYPES 8

int main(void)
{
	const struct voe_ecs_key *keys[TYPES] = {
		&voe_scene_transform_key, &voe_scene_identity_key,
		&voe_scene_light_key,	  &voe_scene_camera_key,
		&voe_3d_mesh_key,	  &voe_3d_material_key,
		&voe_3d_panel_key,	  &voe_3d_shape_key,
	};
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_type types[TYPES];

	for (int i = 0; i < TYPES; i++)
		types[i] = voe_ecs_component_type(world, keys[i]);

	for (int i = 0; i < TYPES; i++)
		for (int j = i + 1; j < TYPES; j++)
			VOE_TEST_CHECK(types[i].value != types[j].value);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
