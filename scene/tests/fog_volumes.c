// The fog volumes: what registration tells a tool (the menu path, the
// transform needed, the default row of 0493 point 1, the six fields in order
// with `size` FLOAT3 and the rest FLOAT32, the drift table runtime-only), the
// ranges of thickness 0 to 4, patchiness and godrays_strength 0 to 1 and
// wind_direction 0 to 360 °, wind_speed none, that a row arrives as given,
// that a replace changes `thickness` only when the system runs, that
// thickness 4 is taken, that each refusal keeps the row, that a run adds a
// drift at 0, that the wind moves it by speed × seconds towards its direction, that
// removing the row drops its drift on the next run, and that adding to a dead
// entity is false.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one through shows as that one.
//
// THE DESCRIPTION IS SWITCHED ON HERE, as scene/tests/clouds.c does, so the
// field list is checked in every build.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/fog_volume_component.h>
#include <scene/fog_volume_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define VOLUMES 2

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = VOLUMES,
		.component_types = 3,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, VOLUMES);
	voe_scene_fog_volume_register(world, VOLUMES);
	return world;
}

// No field the default's, so a default copied in shows as a wrong number.
static voe_scene_fog_volume known(void)
{
	return (voe_scene_fog_volume){ .size = { 8.0f, 3.0f, 6.0f },
				       .thickness = 0.25f,
				       .patchiness = 0.5f,
				       .wind_direction = 45.0f,
				       .wind_speed = 3.0f,
				       .godrays_strength = 0.75f };
}

static void check_fog(const voe_scene_fog_volume *read,
		      voe_scene_fog_volume expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->size.x, expected.size.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->size.y, expected.size.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->size.z, expected.size.z, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->thickness, expected.thickness, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->patchiness, expected.patchiness, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->wind_direction, expected.wind_direction,
			     0.0f);
	VOE_TEST_CHECK_FLOAT(read->wind_speed, expected.wind_speed, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->godrays_strength, expected.godrays_strength,
			     0.0f);
}

static void check_fields(const voe_base_struct_description *description)
{
	static const char *const names[] = { "size",	       "thickness",
					     "patchiness",     "wind_direction",
					     "wind_speed",     "godrays_strength" };

	VOE_TEST_CHECK_INT(description->field_count, 6);
	if (description->field_count != 6)
		return;
	for (uint32_t i = 0; i < 6; i++) {
		VOE_TEST_CHECK(strcmp(description->fields[i].name, names[i]) ==
			       0);
		VOE_TEST_CHECK_INT(description->fields[i].kind,
				   i == 0 ? VOE_BASE_FIELD_FLOAT3 :
					    VOE_BASE_FIELD_FLOAT32);
		VOE_TEST_CHECK(!description->fields[i].read_only);
	}
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
		voe_scene_fog_volume_description();

	check_range(description, "thickness", 0.0, 4.0, "");
	check_range(description, "patchiness", 0.0, 1.0, "");
	check_range(description, "godrays_strength", 0.0, 1.0, "");
	check_range(description, "wind_direction", 0.0, 360.0, "°");
	VOE_TEST_CHECK(voe_base_ranges_find(description, "wind_speed") == NULL);
}

static void registration_says_what_fog_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_fog_volume_key);
	voe_ecs_type drifts =
		voe_ecs_component_type(world, &voe_scene_fog_volume_drift_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(
		(long long)replace.row_offset,
		(long long)offsetof(voe_scene_fog_volume_intent, fog_volume));
	check_fog(voe_ecs_component_default(world, type),
		  (voe_scene_fog_volume){ .size = { 40.0f, 10.0f, 40.0f },
					  .thickness = 0.4f,
					  .patchiness = 0.6f,
					  .wind_direction = 0.0f,
					  .wind_speed = 1.0f,
					  .godrays_strength = 1.0f });
	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Fog volume") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));
	VOE_TEST_CHECK(voe_ecs_component_runtime_only(world, drifts));
	VOE_TEST_CHECK(voe_ecs_component_menu(world, drifts) == NULL);
	VOE_TEST_CHECK(!voe_ecs_component_replace(world, drifts).set);

	check_fields(voe_scene_fog_volume_description());
}

static void a_volume_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_get(world, valley) == NULL);
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, known()));

	check_fog(voe_scene_fog_volume_get(world, valley), known());
	VOE_TEST_CHECK_INT(voe_scene_fog_volume_count(world), 1);
	VOE_TEST_CHECK(voe_scene_fog_volume_rows(world) ==
		       voe_scene_fog_volume_get(world, valley));
	VOE_TEST_CHECK_INT(voe_scene_fog_volume_entities(world)[0].index,
			   valley.index);
}

static void a_replace_lands_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };
	voe_scene_fog_volume thicker = known();

	thicker.thickness = 0.9f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, known()));
	VOE_TEST_CHECK(voe_scene_fog_volume_submit(
		world, (voe_scene_fog_volume_intent){ .entity = valley,
						      .fog_volume = thicker }));
	check_fog(voe_scene_fog_volume_get(world, valley), known());
	voe_scene_fog_volume_system_run(world, 0.0f);
	check_fog(voe_scene_fog_volume_get(world, valley), thicker);
}

// The old top, 1, is now an ordinary value; 4 is the new top, taken both ways.
static void thickness_four_is_taken(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity added = { 0 };
	voe_ecs_entity replaced = { 0 };
	voe_scene_fog_volume thickest = known();

	thickest.thickness = 4.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &added));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, added, thickest));
	check_fog(voe_scene_fog_volume_get(world, added), thickest);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &replaced));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, replaced, known()));
	VOE_TEST_CHECK(voe_scene_fog_volume_submit(
		world, (voe_scene_fog_volume_intent){ .entity = replaced,
						      .fog_volume = thickest }));
	voe_scene_fog_volume_system_run(world, 0.0f);
	check_fog(voe_scene_fog_volume_get(world, replaced), thickest);
}

static void refused_volumes_keep_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };
	voe_scene_fog_volume bad[16];

	for (int i = 0; i < 16; i++)
		bad[i] = known();
	bad[0].size.x = -1.0f;
	bad[1].size.y = -1.0f;
	bad[2].size.z = -1.0f;
	bad[3].size.x = INFINITY;
	bad[4].thickness = -0.1f;
	bad[5].thickness = 4.1f;
	bad[6].patchiness = -0.1f;
	bad[7].patchiness = 1.1f;
	bad[8].godrays_strength = -0.1f;
	bad[9].godrays_strength = 1.1f;
	bad[10].wind_speed = -1.0f;
	bad[11].wind_direction = NAN;
	bad[12].wind_direction = INFINITY;
	bad[13].wind_speed = INFINITY;
	bad[14].thickness = NAN;
	bad[15].size.y = NAN;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, known()));

	for (int i = 0; i < 16; i++) {
		VOE_TEST_CHECK(voe_scene_fog_volume_submit(
			world, (voe_scene_fog_volume_intent){
				       .entity = valley, .fog_volume = bad[i] }));
		voe_scene_fog_volume_system_run(world, 0.0f);
		check_fog(voe_scene_fog_volume_get(world, valley), known());
	}
}

static void a_run_adds_a_drift_at_zero(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };
	const voe_scene_fog_volume_drift *drift;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, known()));
	VOE_TEST_CHECK(voe_scene_fog_volume_drift_get(world, valley) == NULL);
	voe_scene_fog_volume_system_run(world, 0.0f);

	drift = voe_scene_fog_volume_drift_get(world, valley);
	VOE_TEST_CHECK(drift != NULL);
	if (drift == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(drift->x, 0.0, 0.0);
	VOE_TEST_CHECK_FLOAT(drift->z, 0.0, 0.0);
}

static void the_wind_moves_the_drift(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };
	voe_scene_fog_volume windy = known();
	const voe_scene_fog_volume_drift *drift;

	windy.wind_direction = 90.0f;
	windy.wind_speed = 2.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, windy));
	voe_scene_fog_volume_system_run(world, 0.5f);
	voe_scene_fog_volume_system_run(world, 0.5f);

	drift = voe_scene_fog_volume_drift_get(world, valley);
	VOE_TEST_CHECK(drift != NULL);
	if (drift == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(drift->x, 2.0, 1e-6);
	VOE_TEST_CHECK_FLOAT(drift->z, 0.0, 1e-6);
}

static void removing_the_row_drops_its_drift(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity valley = { 0 };
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_fog_volume_key);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, known()));
	voe_scene_fog_volume_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_scene_fog_volume_drift_get(world, valley) != NULL);

	VOE_TEST_CHECK(voe_ecs_component_remove(world, type, valley));
	voe_scene_fog_volume_system_run(world, 0.0f);
	VOE_TEST_CHECK(voe_scene_fog_volume_drift_get(world, valley) == NULL);
}

static void adding_to_a_dead_entity_is_false(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity gone = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &gone));
	voe_ecs_entity_destroy(world, gone);
	VOE_TEST_CHECK(!voe_scene_fog_volume_add(world, gone, known()));
	VOE_TEST_CHECK_INT(voe_scene_fog_volume_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(512 * 1024);

	registration_says_what_fog_is(arena);
	the_bounded_fields_say_their_ranges();
	a_volume_arrives_as_given(arena);
	a_replace_lands_when_the_system_runs(arena);
	thickness_four_is_taken(arena);
	refused_volumes_keep_the_row(arena);
	a_run_adds_a_drift_at_zero(arena);
	the_wind_moves_the_drift(arena);
	removing_the_row_drops_its_drift(arena);
	adding_to_a_dead_entity_is_false(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
