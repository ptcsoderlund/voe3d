// A project type registered through voe_game_project_component on a game
// world: its menu and default read back, a replace in the layout
// voe_ecs_component_replace gives lands whole, one for a destroyed entity is
// dropped with the queue empty after, and a row of 241 bytes is refused. A
// type registered needing the transform answers it as needed; one with no
// needs answers none. A
// hand-written two-entity prefab spawned lands with its child under its
// root, an unknown name is refused, a remove takes both, and a thousand
// rounds leave the world's entity count where it began.
// Needs no window and no graphics card.
#include <game/project.h>

#include <game/world.h>

#include <base/arena.h>
#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/intent.h>
#include <ecs/structure.h>

#include <testing/test.h>

#include <string.h>

#define SPEED_FIELDS(F, F_READ_ONLY) F(float, metres, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(speed, SPEED_FIELDS)

static const struct voe_ecs_key speed_key = { "speed" };
static const struct voe_ecs_key wide_key = { "wide" };
static const struct voe_ecs_key follow_key = { "follow" };

// Submits a replace of `entity`'s speed as a caller knowing only the layout.
static void submit(voe_ecs_world *world, voe_ecs_type type,
		   voe_ecs_entity entity, float metres)
{
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	unsigned char value[256] = { 0 };
	speed row = { metres };

	VOE_TEST_CHECK(replace.set && replace.value_size <= sizeof(value));
	memcpy(value, &entity, sizeof(entity));
	memcpy(value + replace.row_offset, &row, sizeof(row));
	VOE_TEST_CHECK(voe_ecs_intent_submit(world, replace.intent, value));
}

// A root at `position` and a child one metre over it, as a cook would write.
static bool thing(voe_ecs_world *world, const voe_ecs_entity *entities,
		  voe_math_double3 position, voe_math_quat rotation)
{
	voe_ecs_type transform =
		voe_ecs_component_type(world, &voe_scene_transform_key);

	return voe_ecs_structure_add(
		       world, transform, entities[0],
		       &(voe_scene_transform){ .position = position,
					       .rotation = rotation,
					       .scale = { 1.0f, 1.0f, 1.0f } }) &&
	       voe_ecs_structure_add(
		       world, transform, entities[1],
		       &(voe_scene_transform){ .position = { 0.0, 1.0, 0.0 },
					       .rotation = { 0, 0, 0, 1 },
					       .scale = { 1.0f, 1.0f, 1.0f } }) &&
	       voe_ecs_structure_add(
		       world,
		       voe_ecs_component_type(world, &voe_scene_parent_key),
		       entities[1], &(voe_scene_parent){ entities[0] });
}

static void spawned_and_removed(voe_ecs_world *world)
{
	static const voe_game_prefab table[] = { { "thing", 2, thing } };
	const voe_game_prefabs prefabs = { table, 1 };
	const voe_game_project_step step = { .world = world,
					     .prefabs = &prefabs };
	const voe_math_quat turn = { 0, 0, 0, 1 };
	const uint32_t before = voe_ecs_entity_count(world);
	voe_ecs_entity child = { 0 };
	voe_math_double3 at;
	voe_ecs_entity tree[4];
	voe_ecs_entity root;

	VOE_TEST_CHECK(voe_game_project_spawn(
		&step, "thing", (voe_math_double3){ 1.0, 2.0, 3.0 }, turn, &root));
	voe_ecs_structure_apply(world);
	at = voe_scene_transform_get(world, root)->position;
	VOE_TEST_CHECK(at.x == 1.0 && at.y == 2.0 && at.z == 3.0);
	VOE_TEST_CHECK_INT(voe_scene_parent_tree(world, root, tree, 4), 2);
	child = tree[1];
	VOE_TEST_CHECK(voe_scene_parent_within(world, child, root));
	VOE_TEST_CHECK(!voe_game_project_spawn(
		&step, "nothing", (voe_math_double3){ 0 }, turn, &root));

	VOE_TEST_CHECK(voe_game_project_remove(&step, root));
	voe_ecs_structure_apply(world);
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, root));
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, child));

	for (int i = 0; i < 1000; i++) {
		VOE_TEST_CHECK(voe_game_project_spawn(
			&step, "thing", (voe_math_double3){ 0 }, turn, &root));
		voe_ecs_structure_apply(world);
		VOE_TEST_CHECK(voe_game_project_remove(&step, root));
		voe_ecs_structure_apply(world);
	}
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), before);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 22);
	voe_ecs_world *world = voe_game_world_new(arena);
	const speed start = { 2.0f };
	voe_ecs_entity kept;
	voe_ecs_entity gone;
	voe_ecs_type type;
	voe_ecs_type needed;

	VOE_TEST_CHECK(voe_game_project_component(
		world, &(voe_game_project_type){
			       &speed_key, sizeof(speed), 4,
			       VOE_GAME_PROJECT_DESCRIPTION(speed), &start,
			       "Game / Speed", NULL }));
	type = voe_ecs_component_type(world, &speed_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Game / Speed") == 0);
	VOE_TEST_CHECK_FLOAT(
		((const speed *)voe_ecs_component_default(world, type))->metres,
		2.0f, 0.0f);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &kept));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, kept, &start));
	VOE_TEST_CHECK(voe_ecs_component_add(world, type, gone, &start));
	submit(world, type, kept, 7.5f);
	submit(world, type, gone, 9.0f);
	voe_ecs_entity_destroy(world, gone);
	voe_game_project_replaces_apply(world);

	VOE_TEST_CHECK_FLOAT(
		((const speed *)voe_ecs_component_get(world, type, kept))->metres,
		7.5f, 0.0f);
	VOE_TEST_CHECK_INT(voe_ecs_component_count(world, type), 1);
	VOE_TEST_CHECK_INT(
		voe_ecs_intent_count(
			world, voe_ecs_component_replace(world, type).intent),
		0);

	VOE_TEST_CHECK(!voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_game_project_component(
		world, &(voe_game_project_type){
			       &follow_key, sizeof(speed), 4,
			       VOE_GAME_PROJECT_DESCRIPTION(speed), NULL, NULL,
			       &voe_scene_transform_key }));
	VOE_TEST_CHECK(voe_ecs_component_needs(
		world, voe_ecs_component_type(world, &follow_key), &needed));
	VOE_TEST_CHECK_INT(
		needed.value,
		voe_ecs_component_type(world, &voe_scene_transform_key).value);

	VOE_TEST_CHECK(!voe_game_project_component(
		world, &(voe_game_project_type){ &wide_key,
						 VOE_GAME_PROJECT_ROW + 1, 4,
						 &voe_ecs_runtime_only, NULL,
						 NULL, NULL }));

	spawned_and_removed(world);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
