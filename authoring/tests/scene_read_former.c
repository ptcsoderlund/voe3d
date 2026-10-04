// The reader's former names (0353), proven on scene's light blocker, whose
// `kind` became `block`: a scene saved with `kind = 0`, `1` and `2` reads
// as All, Fill and Direct; written back it says `block =` and no `kind =`,
// so a file is upgraded on its next save; a section saying both keeps
// `block`'s value whichever line comes first; and a blocker saying neither
// reads All.
//
// Each missing field and each section saying both names prints a warning to
// stderr; that is the report doing its job, not a failure.
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <string.h>

#define ENTITIES 8

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 8,
		.intent_types = 8,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	voe_scene_light_blocker_register(world, ENTITIES);
	return world;
}

// The blocker on the entity with authored id `id`, or NULL.
static const voe_scene_light_blocker *blocker_of(const voe_ecs_world *world,
						 uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id == id)
			return voe_scene_light_blocker_get(world, entities[i]);
	return NULL;
}

static voe_ecs_world *read_scene(voe_base_arena *arena, const char *text)
{
	voe_ecs_world *world = world_of(arena);
	voe_authoring_kept kept = { 0 };

	VOE_TEST_CHECK(voe_authoring_scene_read(text, strlen(text), world, arena,
						&kept));
	return world;
}

static void check_block(const voe_ecs_world *world, uint64_t id,
			uint32_t block)
{
	const voe_scene_light_blocker *blocker = blocker_of(world, id);

	VOE_TEST_CHECK(blocker != NULL);
	if (blocker != NULL)
		VOE_TEST_CHECK_INT(blocker->block, block);
}

static const char saved_with_kind[] = "[1]\n"
				      "name = \"A\"\n"
				      "folded = false\n"
				      "[1.voe_scene_light_blocker]\n"
				      "size = [1, 1, 1]\n"
				      "kind = 0\n"
				      "[2]\n"
				      "name = \"B\"\n"
				      "folded = false\n"
				      "[2.voe_scene_light_blocker]\n"
				      "size = [1, 1, 1]\n"
				      "kind = 1\n"
				      "[3]\n"
				      "name = \"C\"\n"
				      "folded = false\n"
				      "[3.voe_scene_light_blocker]\n"
				      "size = [1, 1, 1]\n"
				      "kind = 2\n";

static void test_kind_reads_as_block(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, saved_with_kind);

	check_block(world, 1, VOE_SCENE_LIGHT_BLOCKER_ALL);
	check_block(world, 2, VOE_SCENE_LIGHT_BLOCKER_FILL);
	check_block(world, 3, VOE_SCENE_LIGHT_BLOCKER_DIRECT);
	voe_base_arena_destroy(arena);
}

static void test_written_back_says_block(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, saved_with_kind);
	voe_authoring_text out = { 0 };

	VOE_TEST_CHECK(voe_authoring_scene_write(world, NULL, arena, &out));
	if (out.text != NULL) {
		char *text = voe_base_arena_push(arena, out.size + 1);

		memcpy(text, out.text, out.size);
		text[out.size] = '\0';
		VOE_TEST_CHECK(strstr(text, "block = 2") != NULL);
		VOE_TEST_CHECK(strstr(text, "kind =") == NULL);
	}
	voe_base_arena_destroy(arena);
}

static void test_current_name_wins_either_order(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"A\"\n"
					   "folded = false\n"
					   "[1.voe_scene_light_blocker]\n"
					   "block = 2\n"
					   "kind = 1\n"
					   "[2]\n"
					   "name = \"B\"\n"
					   "folded = false\n"
					   "[2.voe_scene_light_blocker]\n"
					   "kind = 1\n"
					   "block = 2\n");

	check_block(world, 1, VOE_SCENE_LIGHT_BLOCKER_DIRECT);
	check_block(world, 2, VOE_SCENE_LIGHT_BLOCKER_DIRECT);
	voe_base_arena_destroy(arena);
}

static void test_neither_reads_all(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"A\"\n"
					   "folded = false\n"
					   "[1.voe_scene_light_blocker]\n"
					   "size = [2, 2, 2]\n");

	check_block(world, 1, VOE_SCENE_LIGHT_BLOCKER_ALL);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_kind_reads_as_block();
	test_written_back_says_block();
	test_current_name_wins_either_order();
	test_neither_reads_all();
	return voe_test_result();
}
