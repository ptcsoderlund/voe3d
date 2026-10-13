// The haze: what registration tells a tool (the menu path, no needed type, the
// default row of haze 1000, start 100 and height 1000, the three fields in
// order, `height` last, each FLOAT32), that a row arrives as given, that a
// replace changes `haze` only when the system runs, that a replace with height
// -50 is taken, that a NaN haze, a haze of -1, a start of -1, an infinite
// start, a NaN height and an infinite height are each refused and keep the
// row, and that adding to a dead entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/sky.c does, so the field
// list is checked in every build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/haze_component.h>
#include <scene/haze_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define HAZES 2

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = HAZES,
		.component_types = 1,
		.intent_types = 1,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_haze_register(world, HAZES);
	return world;
}

// No field the default's, so a default copied in shows as a wrong number.
static voe_scene_haze known(void)
{
	return (voe_scene_haze){ .haze = 50.0f, .start = 20.0f, .height = 30.0f };
}

static void check_haze(const voe_scene_haze *read, voe_scene_haze expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->haze, expected.haze, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->start, expected.start, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->height, expected.height, 0.0f);
}

static void registration_says_what_a_haze_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_haze_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *description =
		voe_scene_haze_description();

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_haze_intent, haze));
	check_haze(voe_ecs_component_default(world, type),
		   (voe_scene_haze){ .haze = 1000.0f, .start = 100.0f,
				     .height = 1000.0f });
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Haze") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	VOE_TEST_CHECK_INT(description->field_count, 3);
	if (description->field_count != 3)
		return;
	VOE_TEST_CHECK(strcmp(description->fields[0].name, "haze") == 0);
	VOE_TEST_CHECK_INT(description->fields[0].kind, VOE_BASE_FIELD_FLOAT32);
	VOE_TEST_CHECK(strcmp(description->fields[1].name, "start") == 0);
	VOE_TEST_CHECK_INT(description->fields[1].kind, VOE_BASE_FIELD_FLOAT32);
	VOE_TEST_CHECK(strcmp(description->fields[2].name, "height") == 0);
	VOE_TEST_CHECK_INT(description->fields[2].kind, VOE_BASE_FIELD_FLOAT32);
}

static void a_haze_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_haze_get(world, settings) == NULL);
	VOE_TEST_CHECK(voe_scene_haze_add(world, settings, known()));

	check_haze(voe_scene_haze_get(world, settings), known());
	VOE_TEST_CHECK_INT(voe_scene_haze_count(world), 1);
	VOE_TEST_CHECK(voe_scene_haze_rows(world) ==
		       voe_scene_haze_get(world, settings));
	VOE_TEST_CHECK_INT(voe_scene_haze_entities(world)[0].index,
			   settings.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_haze thicker = known();

	thicker.haze = 10.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_haze_add(world, settings, known()));
	VOE_TEST_CHECK(voe_scene_haze_submit(
		world, (voe_scene_haze_intent){ .entity = settings,
						.haze = thicker }));
	check_haze(voe_scene_haze_get(world, settings), known());
	voe_scene_haze_system_run(world);
	check_haze(voe_scene_haze_get(world, settings), thicker);
}

// A height below the ground is still a height: any finite number is taken.
static void a_negative_height_is_taken(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_haze low = known();

	low.height = -50.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_haze_add(world, settings, known()));
	VOE_TEST_CHECK(voe_scene_haze_submit(
		world, (voe_scene_haze_intent){ .entity = settings,
						.haze = low }));
	voe_scene_haze_system_run(world);
	check_haze(voe_scene_haze_get(world, settings), low);
}

static void a_refused_haze_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_haze bad[6] = { known(), known(), known(),
				  known(), known(), known() };

	bad[0].haze = NAN;
	bad[1].haze = -1.0f;
	bad[2].start = -1.0f;
	bad[3].start = INFINITY;
	bad[4].height = NAN;
	bad[5].height = INFINITY;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_haze_add(world, settings, known()));

	for (int i = 0; i < 6; i++) {
		VOE_TEST_CHECK(voe_scene_haze_submit(
			world, (voe_scene_haze_intent){ .entity = settings,
							.haze = bad[i] }));
		voe_scene_haze_system_run(world);
		check_haze(voe_scene_haze_get(world, settings), known());
	}
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_haze_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_haze_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_haze_is(arena);
	a_haze_arrives_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	a_negative_height_is_taken(arena);
	a_refused_haze_keeps_the_row(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
