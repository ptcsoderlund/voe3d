// The picture: what registration tells a tool (the menu path, no needed type,
// the default row of exposure 0 and Soft, the two fields and the tone names
// "Clip" and "Soft"), exposure's range -16 to 16 stops and none on tone, that
// a row arrives as given, that a replace changes the
// exposure only when the system runs, that a NaN exposure, an exposure of 17
// and a tone of 2 are each refused and keep the row, and that adding to a dead
// entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/sky.c does, so the field
// list and the names are checked in every build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/picture_component.h>
#include <scene/picture_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define PICTURES 2

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = PICTURES,
		.component_types = 1,
		.intent_types = 1,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_picture_register(world, PICTURES);
	return world;
}

// No field the default's, so a default copied in shows as a wrong number.
static voe_scene_picture known(void)
{
	return (voe_scene_picture){ .exposure = -1.5f,
				    .tone = VOE_SCENE_PICTURE_TONE_CLIP };
}

static void check_picture(const voe_scene_picture *read,
			  voe_scene_picture expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->exposure, expected.exposure, 0.0f);
	VOE_TEST_CHECK_INT(read->tone, expected.tone);
}

static void registration_says_what_a_picture_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_picture_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *description =
		voe_scene_picture_description();
	const voe_base_field_names *names;

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_picture_intent,
					       picture));
	check_picture(voe_ecs_component_default(world, type),
		      (voe_scene_picture){
			      .exposure = 0.0f,
			      .tone = VOE_SCENE_PICTURE_TONE_SOFT });
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Picture") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	VOE_TEST_CHECK_INT(description->field_count, 2);
	if (description->field_count != 2)
		return;
	VOE_TEST_CHECK(strcmp(description->fields[0].name, "exposure") == 0);
	VOE_TEST_CHECK_INT(description->fields[0].kind, VOE_BASE_FIELD_FLOAT32);
	VOE_TEST_CHECK(strcmp(description->fields[1].name, "tone") == 0);
	VOE_TEST_CHECK_INT(description->fields[1].kind, VOE_BASE_FIELD_UINT32);

	names = voe_base_names_find(description, "tone");
	VOE_TEST_CHECK(names != NULL);
	if (names == NULL)
		return;
	VOE_TEST_CHECK_INT(names->value_count, 2);
	VOE_TEST_CHECK(strcmp(names->values[VOE_SCENE_PICTURE_TONE_CLIP],
			      "Clip") == 0);
	VOE_TEST_CHECK(strcmp(names->values[VOE_SCENE_PICTURE_TONE_SOFT],
			      "Soft") == 0);
}

// Exposure carries its range either way; tone does not, and its names are
// still there beside it.
static void exposure_says_its_range(void)
{
	const voe_base_struct_description *description =
		voe_scene_picture_description();
	const voe_base_field_range *range =
		voe_base_ranges_find(description, "exposure");

	VOE_TEST_CHECK(voe_base_ranges_find(description, "tone") == NULL);
	VOE_TEST_CHECK(voe_base_names_find(description, "tone") != NULL);
	VOE_TEST_CHECK(range != NULL);
	if (range == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(range->least, -VOE_SCENE_PICTURE_EXPOSURE_MAX, 0.0);
	VOE_TEST_CHECK_FLOAT(range->greatest, VOE_SCENE_PICTURE_EXPOSURE_MAX,
			     0.0);
	VOE_TEST_CHECK(strcmp(range->unit, "stops") == 0);
}

static void a_picture_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_picture_get(world, settings) == NULL);
	VOE_TEST_CHECK(voe_scene_picture_add(world, settings, known()));

	check_picture(voe_scene_picture_get(world, settings), known());
	VOE_TEST_CHECK_INT(voe_scene_picture_count(world), 1);
	VOE_TEST_CHECK(voe_scene_picture_rows(world) ==
		       voe_scene_picture_get(world, settings));
	VOE_TEST_CHECK_INT(voe_scene_picture_entities(world)[0].index,
			   settings.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_picture brighter = known();

	brighter.exposure = 2.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_picture_add(world, settings, known()));
	VOE_TEST_CHECK(voe_scene_picture_submit(
		world, (voe_scene_picture_intent){ .entity = settings,
						   .picture = brighter }));
	check_picture(voe_scene_picture_get(world, settings), known());
	voe_scene_picture_system_run(world);
	check_picture(voe_scene_picture_get(world, settings), brighter);
}

static void a_refused_picture_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_picture bad[3] = { known(), known(), known() };

	bad[0].exposure = NAN;
	bad[1].exposure = 17.0f;
	bad[2].tone = VOE_SCENE_PICTURE_TONE_SOFT + 1u;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_picture_add(world, settings, known()));

	for (int i = 0; i < 3; i++) {
		VOE_TEST_CHECK(voe_scene_picture_submit(
			world, (voe_scene_picture_intent){ .entity = settings,
							   .picture = bad[i] }));
		voe_scene_picture_system_run(world);
		check_picture(voe_scene_picture_get(world, settings), known());
	}
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_picture_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_picture_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_picture_is(arena);
	exposure_says_its_range();
	a_picture_arrives_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	a_refused_picture_keeps_the_row(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
