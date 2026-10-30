// The clouds: what registration tells a tool (the menu path, no needed type,
// the default row of 0492 point 1, the seven fields in order with `kind`
// UINT32 named Flat and Volumetric and the rest FLOAT32, the drift table
// runtime-only), the ranges of coverage and shadow_strength 0 to 1 and
// wind_direction 0 to 360 °, height none, that a row arrives as given, that a replace changes
// `coverage` only when the system runs, that each refusal keeps the row, that
// a run adds a drift at 0, that the wind moves it by speed × seconds towards
// its direction, that removing the row drops its drift on the next run, and
// that adding to a dead entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/haze.c does, so the field
// list and the names are checked in every build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/clouds_component.h>
#include <scene/clouds_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define CLOUDS 2

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = CLOUDS,
		.component_types = 2,
		.intent_types = 1,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_clouds_register(world, CLOUDS);
	return world;
}

// No field the default's, so a default copied in shows as a wrong number.
static voe_scene_clouds known(void)
{
	return (voe_scene_clouds){ .kind = VOE_SCENE_CLOUDS_VOLUMETRIC,
				   .coverage = 0.25f,
				   .height = 800.0f,
				   .thickness = 300.0f,
				   .wind_direction = 90.0f,
				   .wind_speed = 5.0f,
				   .shadow_strength = 0.4f };
}

static void check_clouds(const voe_scene_clouds *read,
			 voe_scene_clouds expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_INT(read->kind, expected.kind);
	VOE_TEST_CHECK_FLOAT(read->coverage, expected.coverage, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->height, expected.height, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->thickness, expected.thickness, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->wind_direction, expected.wind_direction,
			     0.0f);
	VOE_TEST_CHECK_FLOAT(read->wind_speed, expected.wind_speed, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->shadow_strength, expected.shadow_strength,
			     0.0f);
}

static void check_fields(const voe_base_struct_description *description)
{
	static const char *const names[] = { "kind",	   "coverage",
					     "height",	   "thickness",
					     "wind_direction", "wind_speed",
					     "shadow_strength" };
	const voe_base_field_names *kinds;

	VOE_TEST_CHECK_INT(description->field_count, 7);
	if (description->field_count != 7)
		return;
	for (uint32_t i = 0; i < 7; i++) {
		VOE_TEST_CHECK(strcmp(description->fields[i].name, names[i]) ==
			       0);
		VOE_TEST_CHECK_INT(description->fields[i].kind,
				   i == 0 ? VOE_BASE_FIELD_UINT32 :
					    VOE_BASE_FIELD_FLOAT32);
	}

	kinds = voe_base_names_find(description, "kind");
	VOE_TEST_CHECK(kinds != NULL);
	if (kinds == NULL)
		return;
	VOE_TEST_CHECK_INT(kinds->value_count, 2);
	VOE_TEST_CHECK(strcmp(kinds->values[VOE_SCENE_CLOUDS_FLAT], "Flat") ==
		       0);
	VOE_TEST_CHECK(strcmp(kinds->values[VOE_SCENE_CLOUDS_VOLUMETRIC],
			      "Volumetric") == 0);
}

static void check_range(const voe_base_struct_description *description,
			const char *field, double least, double greatest,
			const char *unit)
{
	const voe_base_field_range *range =
		voe_base_ranges_find(description, field);

	VOE_TEST_CHECK(range != NULL);
	if (range == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(range->least, least, 0.0);
	VOE_TEST_CHECK_FLOAT(range->greatest, greatest, 0.0);
	VOE_TEST_CHECK(strcmp(range->unit, unit) == 0);
}

static void the_bounded_fields_say_their_ranges(void)
{
	const voe_base_struct_description *description =
		voe_scene_clouds_description();

	check_range(description, "coverage", 0.0, 1.0, "");
	check_range(description, "shadow_strength", 0.0, 1.0, "");
	check_range(description, "wind_direction", 0.0, 360.0, "°");
	VOE_TEST_CHECK(voe_base_ranges_find(description, "height") == NULL);
}

static void registration_says_what_clouds_are(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_clouds_key);
	voe_ecs_type drifts =
		voe_ecs_component_type(world, &voe_scene_clouds_drift_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_clouds_intent, clouds));
	check_clouds(voe_ecs_component_default(world, type),
		     (voe_scene_clouds){ .kind = VOE_SCENE_CLOUDS_FLAT,
					 .coverage = 0.5f,
					 .height = 1500.0f,
					 .thickness = 600.0f,
					 .wind_direction = 0.0f,
					 .wind_speed = 10.0f,
					 .shadow_strength = 0.6f });
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Clouds") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(world, drifts));
	VOE_TEST_CHECK(voe_ecs_component_menu(world, drifts) == NULL);
	VOE_TEST_CHECK(!voe_ecs_component_replace(world, drifts).set);

	check_fields(voe_scene_clouds_description());
}

static void clouds_arrive_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_get(world, settings) == NULL);
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, known()));

	check_clouds(voe_scene_clouds_get(world, settings), known());
	VOE_TEST_CHECK_INT(voe_scene_clouds_count(world), 1);
	VOE_TEST_CHECK(voe_scene_clouds_rows(world) ==
		       voe_scene_clouds_get(world, settings));
	VOE_TEST_CHECK_INT(voe_scene_clouds_entities(world)[0].index,
			   settings.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_clouds denser = known();

	denser.coverage = 0.9f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, known()));
	VOE_TEST_CHECK(voe_scene_clouds_submit(
		world, (voe_scene_clouds_intent){ .entity = settings,
						  .clouds = denser }));
	check_clouds(voe_scene_clouds_get(world, settings), known());
	voe_scene_clouds_system_run(world, 0.0f);
	check_clouds(voe_scene_clouds_get(world, settings), denser);
}

static void refused_clouds_keep_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_clouds bad[12];

	for (int i = 0; i < 12; i++)
		bad[i] = known();
	bad[0].coverage = NAN;
	bad[1].height = INFINITY;
	bad[2].wind_direction = NAN;
	bad[3].kind = VOE_SCENE_CLOUDS_VOLUMETRIC + 1;
	bad[4].coverage = -0.1f;
	bad[5].coverage = 1.1f;
	bad[6].shadow_strength = -0.1f;
	bad[7].shadow_strength = 1.1f;
	bad[8].thickness = -1.0f;
	bad[9].wind_speed = -1.0f;
	bad[10].thickness = INFINITY;
	bad[11].wind_speed = NAN;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, known()));

	for (int i = 0; i < 12; i++) {
		VOE_TEST_CHECK(voe_scene_clouds_submit(
			world, (voe_scene_clouds_intent){ .entity = settings,
							  .clouds = bad[i] }));
		voe_scene_clouds_system_run(world, 0.0f);
		check_clouds(voe_scene_clouds_get(world, settings), known());
	}
}

// Runs `runs` steps of `seconds` with the wind given, from a fresh row, and
// checks where the drift ends.
static void check_wind(voe_base_arena *arena, float direction, float speed,
		       int runs, float seconds, double x, double z)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_clouds windy = known();
	const voe_scene_clouds_drift *drift;

	windy.wind_direction = direction;
	windy.wind_speed = speed;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, windy));
	for (int i = 0; i < runs; i++)
		voe_scene_clouds_system_run(world, seconds);

	drift = voe_scene_clouds_drift_get(world, settings);
	VOE_TEST_CHECK(drift != NULL);
	if (drift == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(drift->x, x, 1e-6);
	VOE_TEST_CHECK_FLOAT(drift->z, z, 1e-6);
}

static void the_wind_moves_the_drift(voe_base_arena *arena)
{
	check_wind(arena, 90.0f, 7.0f, 1, 0.0f, 0.0, 0.0);
	check_wind(arena, 90.0f, 10.0f, 2, 0.5f, 10.0, 0.0);
	check_wind(arena, 0.0f, 4.0f, 1, 0.5f, 0.0, -2.0);
}

static void removing_the_row_drops_its_drift(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_clouds_key);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_get(world, settings) == NULL);
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, known()));
	voe_scene_clouds_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_scene_clouds_drift_get(world, settings) != NULL);

	VOE_TEST_CHECK(voe_ecs_component_remove(world, type, settings));
	voe_scene_clouds_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_scene_clouds_drift_get(world, settings) == NULL);
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_clouds_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_clouds_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	registration_says_what_clouds_are(arena);
	the_bounded_fields_say_their_ranges();
	clouds_arrive_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	refused_clouds_keep_the_row(arena);
	the_wind_moves_the_drift(arena);
	removing_the_row_drops_its_drift(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
