// The reader's unsaid row (0324): that a field a section does not mention is
// read from the type's unsaid row when it has one, from its default row when it
// has none, and that a field the section says keeps what it says. Proven on a
// test-only type whose two rows differ in one field, and on scene's real light,
// whose unsaid row casts shadows where its default does not — so a light saved
// before 049 keeps its sun's shadows. A light as 047 saved it, bounces 1 and no
// bounce_strength, reads bounces 1 and strength 1, so its bounce stays (0326).
//
// THE TEST-ONLY TYPES ARE DECLARED THROUGH VOE_BASE_DESCRIBE_STRUCT, as
// scene_read.c's test declares its own; this file is its own program, kept apart
// from that one's length.
//
// Each missing field prints a warning to stderr; that is the report doing its
// job, not a failure.
#include <authoring/scene_read.h>

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <string.h>

#define ENTITIES 8

#define TOGGLED_FIELDS(F, F_READ_ONLY) \
	F(float, weight, FLOAT32)       \
	F(bool, on, BOOL)

VOE_BASE_DESCRIBE_STRUCT(toggled, TOGGLED_FIELDS)

// Same fields, and a default row but no unsaid row.
VOE_BASE_DESCRIBE_STRUCT(plain, TOGGLED_FIELDS)

static const struct voe_ecs_key toggled_key = { "test_toggled" };
static const struct voe_ecs_key plain_key = { "test_plain" };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES,
		.component_types = 8,
		.intent_types = 8,
	});

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_identity_register(world, ENTITIES);
	voe_scene_light_register(world, ENTITIES);

	voe_ecs_type toggled_type = voe_ecs_component_register(
		world, &toggled_key, sizeof(toggled), ENTITIES,
		toggled_description());
	voe_ecs_type plain_type = voe_ecs_component_register(
		world, &plain_key, sizeof(plain), ENTITIES, plain_description());

	voe_ecs_component_default_set(world, toggled_type,
				      &(toggled){ .weight = 2.0f, .on = false });
	voe_ecs_component_unsaid_set(world, toggled_type,
				     &(toggled){ .weight = 2.0f, .on = true });
	voe_ecs_component_default_set(world, plain_type,
				      &(plain){ .weight = 3.0f, .on = false });
	return world;
}

// The row of `key` on the entity with authored id `id`, or NULL.
static const void *row_of(const voe_ecs_world *world,
			  const struct voe_ecs_key *key, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);

	for (uint32_t i = 0; i < voe_scene_identity_count(world); i++)
		if (rows[i].id == id)
			return voe_ecs_component_get(
				world, voe_ecs_component_type(world, key),
				entities[i]);
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

static void test_missing_field_reads_unsaid(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"A\"\n"
					   "folded = false\n"
					   "[1.test_toggled]\n"
					   "weight = 5\n");
	const toggled *row = row_of(world, &toggled_key, 1);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK(row->on);
		VOE_TEST_CHECK(row->weight == 5.0f);
	}
	voe_base_arena_destroy(arena);
}

static void test_no_unsaid_row_reads_default(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"A\"\n"
					   "folded = false\n"
					   "[1.test_plain]\n"
					   "on = true\n");
	const plain *row = row_of(world, &plain_key, 1);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK(row->weight == 3.0f);
		VOE_TEST_CHECK(row->on);
	}
	voe_base_arena_destroy(arena);
}

static void test_said_field_keeps_what_it_says(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"A\"\n"
					   "folded = false\n"
					   "[1.test_toggled]\n"
					   "weight = 1\n"
					   "on = false\n");
	const toggled *row = row_of(world, &toggled_key, 1);

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK(!row->on);
		VOE_TEST_CHECK(row->weight == 1.0f);
	}
	voe_base_arena_destroy(arena);
}

static void test_light_cast_shadows(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"Old sun\"\n"
					   "folded = false\n"
					   "[1.voe_scene_light]\n"
					   "intensity = 2\n"
					   "[2]\n"
					   "name = \"New sun\"\n"
					   "folded = false\n"
					   "[2.voe_scene_light]\n"
					   "intensity = 2\n"
					   "cast_shadows = false\n");
	const voe_scene_light *old = row_of(world, &voe_scene_light_key, 1);
	const voe_scene_light *said = row_of(world, &voe_scene_light_key, 2);

	VOE_TEST_CHECK(old != NULL && said != NULL);
	if (old != NULL && said != NULL) {
		VOE_TEST_CHECK(old->cast_shadows);
		VOE_TEST_CHECK(!said->cast_shadows);
		VOE_TEST_CHECK(old->intensity == 2.0f);
	}
	voe_base_arena_destroy(arena);
}

// A light as 047 saved it: bounces said, bounce_strength not yet a field.
static void test_light_047_bounce(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_ecs_world *world = read_scene(arena, "[1]\n"
					   "name = \"047 sun\"\n"
					   "folded = false\n"
					   "[1.voe_scene_light]\n"
					   "intensity = 2\n"
					   "bounces = 1\n");
	const voe_scene_light *light = row_of(world, &voe_scene_light_key, 1);

	VOE_TEST_CHECK(light != NULL);
	if (light != NULL) {
		VOE_TEST_CHECK_INT(light->bounces, 1);
		VOE_TEST_CHECK(light->bounce_strength == 1.0f);
	}
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_missing_field_reads_unsaid();
	test_no_unsaid_row_reads_default();
	test_said_field_keeps_what_it_says();
	test_light_cast_shadows();
	test_light_047_bounce();
	return voe_test_result();
}
