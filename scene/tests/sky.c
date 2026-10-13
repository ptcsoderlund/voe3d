// The sky: what registration tells a tool (the menu path, no needed type, the
// default row with strength 1, and the five fields in order, strength last,
// the colours COLOUR), that a row arrives as given, that a replace changes the
// horizon only when the system runs, that a NaN zenith, a negative ground
// channel, a horizon height of 1.5, a strength of -1 and an infinite strength
// are each refused and keep the row, and that adding to a dead entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/light_blocker.c does, so
// the field list is checked in every build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/sky_component.h>
#include <scene/sky_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define SKIES 2

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = SKIES,
		.component_types = 1,
		.intent_types = 1,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_sky_register(world, SKIES);
	return world;
}

// No field the default's, so a default copied in shows as a wrong number.
static voe_scene_sky known(void)
{
	return (voe_scene_sky){ .zenith = { 0.1f, 0.2f, 0.3f },
				.horizon = { 0.4f, 0.5f, 0.6f },
				.ground = { 0.7f, 0.8f, 0.9f },
				.horizon_height = 0.75f,
				.strength = 2.5f };
}

static void check_colour(voe_math_float3 read, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(read.x, expected.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read.y, expected.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read.z, expected.z, 0.0f);
}

static void check_sky(const voe_scene_sky *read, voe_scene_sky expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	check_colour(read->zenith, expected.zenith);
	check_colour(read->horizon, expected.horizon);
	check_colour(read->ground, expected.ground);
	VOE_TEST_CHECK_FLOAT(read->horizon_height, expected.horizon_height,
			     0.0f);
	VOE_TEST_CHECK_FLOAT(read->strength, expected.strength, 0.0f);
}

static void check_field(const voe_base_field_description *field,
			const char *name, voe_base_field_kind kind)
{
	VOE_TEST_CHECK(strcmp(field->name, name) == 0);
	VOE_TEST_CHECK_INT(field->kind, kind);
}

static void registration_says_what_a_sky_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_sky_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *description =
		voe_scene_sky_description();

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_sky_intent, sky));
	check_sky(voe_ecs_component_default(world, type),
		  (voe_scene_sky){ .zenith = { 0.15f, 0.35f, 0.80f },
				   .horizon = { 0.60f, 0.75f, 0.90f },
				   .ground = { 0.20f, 0.18f, 0.16f },
				   .horizon_height = 0.25f,
				   .strength = 1.0f });
	VOE_TEST_CHECK(!voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Sky") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	VOE_TEST_CHECK_INT(description->field_count, 5);
	if (description->field_count != 5)
		return;
	check_field(&description->fields[0], "zenith", VOE_BASE_FIELD_COLOUR);
	check_field(&description->fields[1], "horizon", VOE_BASE_FIELD_COLOUR);
	check_field(&description->fields[2], "ground", VOE_BASE_FIELD_COLOUR);
	check_field(&description->fields[3], "horizon_height",
		    VOE_BASE_FIELD_FLOAT32);
	check_field(&description->fields[4], "strength",
		    VOE_BASE_FIELD_FLOAT32);
}

static void a_sky_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_sky_get(world, settings) == NULL);
	VOE_TEST_CHECK(voe_scene_sky_add(world, settings, known()));

	check_sky(voe_scene_sky_get(world, settings), known());
	VOE_TEST_CHECK_INT(voe_scene_sky_count(world), 1);
	VOE_TEST_CHECK(voe_scene_sky_rows(world) ==
		       voe_scene_sky_get(world, settings));
	VOE_TEST_CHECK_INT(voe_scene_sky_entities(world)[0].index,
			   settings.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_sky dusk = known();

	dusk.horizon = (voe_math_float3){ 0.9f, 0.5f, 0.3f };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_sky_add(world, settings, known()));
	VOE_TEST_CHECK(voe_scene_sky_submit(
		world, (voe_scene_sky_intent){ .entity = settings, .sky = dusk }));
	check_sky(voe_scene_sky_get(world, settings), known());
	voe_scene_sky_system_run(world);
	check_sky(voe_scene_sky_get(world, settings), dusk);
}

static void a_refused_sky_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity settings = { 0 };
	voe_scene_sky bad[5] = { known(), known(), known(), known(), known() };

	bad[0].zenith.y = NAN;
	bad[1].ground.z = -0.1f;
	bad[2].horizon_height = 1.5f;
	bad[3].strength = -1.0f;
	bad[4].strength = INFINITY;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_sky_add(world, settings, known()));

	for (int i = 0; i < 5; i++) {
		VOE_TEST_CHECK(voe_scene_sky_submit(
			world, (voe_scene_sky_intent){ .entity = settings,
						       .sky = bad[i] }));
		voe_scene_sky_system_run(world);
		check_sky(voe_scene_sky_get(world, settings), known());
	}
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_sky_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_sky_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_sky_is(arena);
	a_sky_arrives_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	a_refused_sky_keeps_the_row(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
