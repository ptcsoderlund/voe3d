// A fresh game world has every one of the seventeen types registered: each public
// key resolves through voe_ecs_component_type, which asserts on a key nothing
// registered, the sixteen answers, the parent, prefab, emitter and particles
// tables' among them, are sixteen different types, and the world counts
// seventeen, the last the transforms' previous table, whose key is scene's own. Needs no window and no graphics card.
//
// The keys come through game/scene.h, the one header a cooked scene.c sees,
// and no component header of their own: a type game/world.h registers that
// scene.h does not declare stops this test compiling. It never calls
// voe_game_scene_build, so it links without a scene.c.
//
// COLLIDERS HAVE THE ROOM OF DRAWN THINGS (0293): a fresh world takes twice
// VOE_GAME_WORLD_AUTHORED new entities, each a transform then a collider
// queued through the structural queue, and has every collider after the apply.
#include <game/scene.h>
#include <game/world.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <testing/test.h>

// The public keys; the previous table is the one more the world counts.
#define TYPES 16

// More colliders than the authored room, as spawned things carry.
#define SPAWNED (2 * VOE_GAME_WORLD_AUTHORED)

// A fresh world, SPAWNED entities each given a default transform and a default
// collider through the structural queue, applied: every collider is there.
static void colliders_case(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_ecs_type transform =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const voe_ecs_type collider =
		voe_ecs_component_type(world, &voe_physics_collider_key);

	for (int i = 0; i < SPAWNED; i++) {
		voe_ecs_entity entity;

		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_ecs_structure_add(
			world, transform, entity,
			voe_ecs_component_default(world, transform)));
		VOE_TEST_CHECK(voe_ecs_structure_add(
			world, collider, entity,
			voe_ecs_component_default(world, collider)));
	}
	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(voe_ecs_component_count(world, collider) == SPAWNED);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	const struct voe_ecs_key *keys[TYPES] = {
		&voe_scene_transform_key, &voe_scene_identity_key,
		&voe_scene_light_key,	  &voe_scene_camera_key,
		&voe_3d_mesh_key,	  &voe_3d_material_key,
		&voe_3d_panel_key,	  &voe_3d_shape_key,
		&voe_physics_collider_key, &voe_physics_body_key,
		&voe_3d_model_key,	  &voe_scene_parent_key,
		&voe_scene_prefab_key,	  &voe_scene_prefab_part_key,
		&voe_3d_emitter_key,	  &voe_3d_particles_key,
	};
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_type types[TYPES];

	for (int i = 0; i < TYPES; i++)
		types[i] = voe_ecs_component_type(world, keys[i]);

	for (int i = 0; i < TYPES; i++)
		for (int j = i + 1; j < TYPES; j++)
			VOE_TEST_CHECK(types[i].value != types[j].value);
	VOE_TEST_CHECK(voe_ecs_component_type_count(world) ==
		       VOE_GAME_WORLD_TYPES);

	voe_base_arena_destroy(arena);
	colliders_case();
	return voe_test_result();
}
