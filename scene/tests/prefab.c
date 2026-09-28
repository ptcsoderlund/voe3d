// The prefab and prefab part rows: that both register on a world with
// transforms, that rows added through the structural queue read back through
// the two getters once applied, that the part table is runtime-only and the
// prefab table is not, that the prefab type has no replace and no menu, and
// that the getters answer NULL for an entity with no row.
#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <scene/prefab_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <string.h>

#define ENTITIES 4

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = ENTITIES,
					 .component_types = 3,
					 .intent_types = 2,
					 .structure_requests = 8,
					 .structure_bytes = 512 });

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_prefab_register(world, ENTITIES);
	return world;
}

static voe_ecs_entity placed(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	return entity;
}

static void rows_read_back_once_applied(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type prefab = voe_ecs_component_type(world, &voe_scene_prefab_key);
	voe_ecs_type part =
		voe_ecs_component_type(world, &voe_scene_prefab_part_key);
	voe_ecs_entity root = placed(world);
	voe_scene_prefab row = { 0 };

	strcpy(row.path, "Assets/tank.prefab");
	VOE_TEST_CHECK(voe_ecs_structure_add(world, prefab, root, &row));
	VOE_TEST_CHECK(voe_ecs_structure_add(
		world, part, root, &(voe_scene_prefab_part){ .instance = root }));
	VOE_TEST_CHECK(voe_scene_prefab_get(world, root) == NULL);
	voe_ecs_structure_apply(world);

	const voe_scene_prefab *read = voe_scene_prefab_get(world, root);
	const voe_scene_prefab_part *named =
		voe_scene_prefab_part_get(world, root);

	VOE_TEST_CHECK(read != NULL &&
		       strcmp(read->path, "Assets/tank.prefab") == 0);
	VOE_TEST_CHECK(named != NULL && named->instance.index == root.index &&
		       named->instance.generation == root.generation);
}

static void part_is_runtime_only_and_prefab_is_shown(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type prefab = voe_ecs_component_type(world, &voe_scene_prefab_key);
	voe_ecs_type part =
		voe_ecs_component_type(world, &voe_scene_prefab_part_key);

	VOE_TEST_CHECK(voe_ecs_component_runtime_only(world, part));
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, prefab));
	VOE_TEST_CHECK(!voe_ecs_component_replace(world, prefab).set);
	VOE_TEST_CHECK(voe_ecs_component_menu(world, prefab) == NULL);
}

static void no_row_answers_null(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity bare = placed(world);

	VOE_TEST_CHECK(voe_scene_prefab_get(world, bare) == NULL);
	VOE_TEST_CHECK(voe_scene_prefab_part_get(world, bare) == NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	rows_read_back_once_applied(arena);
	part_is_runtime_only_and_prefab_is_shown(arena);
	no_row_answers_null(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
