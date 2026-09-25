// A project type registered through voe_game_project_component on a game
// world: its menu and default read back, a replace in the layout
// voe_ecs_component_replace gives lands whole, one for a destroyed entity is
// dropped with the queue empty after, and a row of 241 bytes is refused.
// Needs no window and no graphics card.
#include <game/project.h>

#include <game/world.h>

#include <base/arena.h>
#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/intent.h>

#include <testing/test.h>

#include <string.h>

#define SPEED_FIELDS(F, F_READ_ONLY) F(float, metres, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(speed, SPEED_FIELDS)

static const struct voe_ecs_key speed_key = { "speed" };
static const struct voe_ecs_key wide_key = { "wide" };

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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 22);
	voe_ecs_world *world = voe_game_world_new(arena);
	const speed start = { 2.0f };
	voe_ecs_entity kept;
	voe_ecs_entity gone;
	voe_ecs_type type;

	VOE_TEST_CHECK(voe_game_project_component(
		world, &(voe_game_project_type){
			       &speed_key, sizeof(speed), 4,
			       VOE_GAME_PROJECT_DESCRIPTION(speed), &start,
			       "Game / Speed" }));
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

	VOE_TEST_CHECK(!voe_game_project_component(
		world, &(voe_game_project_type){ &wide_key,
						 VOE_GAME_PROJECT_ROW + 1, 4,
						 &voe_ecs_runtime_only, NULL,
						 NULL }));

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
