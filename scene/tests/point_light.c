// The point light: what registration tells a tool (falloff 1 by default), that
// a refused replace keeps the row, a falloff past either bound included, that an
// accepted one changes falloff and intensity after one run, and that a falloff
// of exactly either bound is accepted. Cast shadows is off by default and lands
// either way. Bounces and bounce strength follow it, last, defaulting to 0 and
// 1; bounces is named with the sun's "0" to "3", lands at 3 and is refused at 4,
// and a negative strength is refused.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one field through shows as that field.
//
// THE NUMBERS ARE CHOSEN TO BE EXACT, so every field is compared with no
// tolerance.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/transform.c gives at length; only this file's own copy is read.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <string.h>

#define LIGHTS 4

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = LIGHTS,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, LIGHTS);
	voe_scene_point_light_register(world, LIGHTS);
	return world;
}

// No field white, one or nought, so a field copied from the wrong place shows.
static voe_scene_point_light lamp_light(void)
{
	return (voe_scene_point_light){
		.colour = { 1.0f, 0.8f, 0.5f },
		.intensity = 2.0f,
		.range = 3.0f,
		.falloff = 1.5f,
		.bounces = 2,
		.bounce_strength = 0.75f,
	};
}

static void check_light(const voe_scene_point_light *read,
			voe_scene_point_light expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(read->colour.x, expected.colour.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->colour.y, expected.colour.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->colour.z, expected.colour.z, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->intensity, expected.intensity, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->range, expected.range, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->falloff, expected.falloff, 0.0f);
	VOE_TEST_CHECK(read->cast_shadows == expected.cast_shadows);
	VOE_TEST_CHECK_INT(read->bounces, expected.bounces);
	VOE_TEST_CHECK_FLOAT(read->bounce_strength, expected.bounce_strength,
			     0.0f);
}

static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset, size_t size)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT((long long)actual->size, (long long)size);
	VOE_TEST_CHECK(!actual->read_only);
}

// The one named field: bounces, with the sun's names "0" to "3".
static void check_bounces_names(const voe_base_struct_description *description)
{
	const voe_base_field_names *names =
		voe_base_names_find(description, "bounces");
	const char *const expected[] = { "0", "1", "2", "3" };

	VOE_TEST_CHECK_INT(description->names_count, 1);
	VOE_TEST_CHECK(names != NULL);
	if (names == NULL)
		return;
	VOE_TEST_CHECK(names->values == voe_scene_light_bounces_names);
	VOE_TEST_CHECK_INT(names->value_count, 4);
	if (names->value_count != 4)
		return;
	for (int i = 0; i < 4; i++)
		VOE_TEST_CHECK(strcmp(names->values[i], expected[i]) == 0);
}

// Seven fields in declared order: cast_shadows a BOOL, then bounces a UINT32
// and bounce_strength a FLOAT32 last.
static void the_description_lists_the_bounce_last(void)
{
	const voe_base_struct_description *description =
		voe_scene_point_light_description();
	const voe_base_field_description *fields = description->fields;

	check_bounces_names(description);
	VOE_TEST_CHECK_INT(description->field_count, 7);
	if (description->field_count != 7)
		return;
	check_field(&fields[0], "colour", VOE_BASE_FIELD_COLOUR,
		    offsetof(voe_scene_point_light, colour),
		    sizeof(voe_math_float3));
	check_field(&fields[1], "intensity", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_point_light, intensity), sizeof(float));
	check_field(&fields[2], "range", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_point_light, range), sizeof(float));
	check_field(&fields[3], "falloff", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_point_light, falloff), sizeof(float));
	check_field(&fields[4], "cast_shadows", VOE_BASE_FIELD_BOOL,
		    offsetof(voe_scene_point_light, cast_shadows), sizeof(bool));
	check_field(&fields[5], "bounces", VOE_BASE_FIELD_UINT32,
		    offsetof(voe_scene_point_light, bounces), sizeof(uint32_t));
	check_field(&fields[6], "bounce_strength", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_point_light, bounce_strength),
		    sizeof(float));
}

static voe_ecs_entity lamp_of(voe_ecs_world *world, voe_scene_point_light light)
{
	voe_ecs_entity lamp = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &lamp));
	VOE_TEST_CHECK(voe_scene_point_light_add(world, lamp, light));
	return lamp;
}

static void replace_and_run(voe_ecs_world *world, voe_ecs_entity lamp,
			    voe_scene_point_light light)
{
	VOE_TEST_CHECK(voe_scene_point_light_submit(
		world, (voe_scene_point_light_intent){ .entity = lamp,
						       .light = light }));
	voe_scene_point_light_system_run(world);
}

// The replace, the default row (white, 1, 5 m, falloff 1, no shadow, no
// bounces at strength 1), the transform it needs, the menu path, and the table
// saved, not runtime-only.
static void registration_says_what_a_point_light_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type =
		voe_ecs_component_type(world, &voe_scene_point_light_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	voe_ecs_type needed = { 0 };

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_point_light_intent,
					       light));
	check_light(voe_ecs_component_default(world, type),
		    (voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					     .intensity = 1.0f,
					     .range = 5.0f,
					     .falloff = 1.0f,
					     .cast_shadows = false,
					     .bounces = 0,
					     .bounce_strength = 1.0f });
	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Point light") == 0);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));
}

// Each bad number on its own: the row stays what it was.
static void a_refused_replace_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, lamp_light());
	voe_scene_point_light bad[14];

	for (int i = 0; i < 14; i++)
		bad[i] = lamp_light();
	bad[0].colour.x = 1.5f;
	bad[1].colour.y = -0.1f;
	bad[2].colour.z = NAN;
	bad[3].intensity = -1.0f;
	bad[4].intensity = INFINITY;
	bad[5].range = 0.0f;
	bad[6].range = -1.0f;
	bad[7].range = NAN;
	bad[8].falloff = 0.2f;
	bad[9].falloff = 4.5f;
	bad[10].falloff = NAN;
	bad[11].bounces = VOE_SCENE_LIGHT_BOUNCES_MAX + 1;
	bad[12].bounce_strength = -1.0f;
	bad[13].bounce_strength = INFINITY;

	for (int i = 0; i < 14; i++) {
		replace_and_run(world, lamp, bad[i]);
		check_light(voe_scene_point_light_get(world, lamp),
			    lamp_light());
	}
}

// Falloff and intensity both change, and only once the system has run.
static void an_accepted_replace_lands_after_one_run(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, lamp_light());
	voe_scene_point_light changed = lamp_light();

	changed.intensity = 0.5f;
	changed.falloff = 3.0f;
	VOE_TEST_CHECK(voe_scene_point_light_submit(
		world, (voe_scene_point_light_intent){ .entity = lamp,
						       .light = changed }));
	check_light(voe_scene_point_light_get(world, lamp), lamp_light());
	voe_scene_point_light_system_run(world);
	check_light(voe_scene_point_light_get(world, lamp), changed);
}

static void a_falloff_at_either_bound_is_accepted(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, lamp_light());
	voe_scene_point_light least = lamp_light();
	voe_scene_point_light most = lamp_light();

	least.falloff = VOE_SCENE_POINT_LIGHT_FALLOFF_LEAST;
	most.falloff = VOE_SCENE_POINT_LIGHT_FALLOFF_MOST;
	replace_and_run(world, lamp, least);
	check_light(voe_scene_point_light_get(world, lamp), least);
	replace_and_run(world, lamp, most);
	check_light(voe_scene_point_light_get(world, lamp), most);
}

static void cast_shadows_lands_either_way(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, lamp_light());
	voe_scene_point_light on = lamp_light();
	voe_scene_point_light off = lamp_light();

	on.cast_shadows = true;
	off.cast_shadows = false;
	replace_and_run(world, lamp, on);
	check_light(voe_scene_point_light_get(world, lamp), on);
	replace_and_run(world, lamp, off);
	check_light(voe_scene_point_light_get(world, lamp), off);
}

// Three bounces land; four, past the sun's maximum, are refused and three stay.
static void bounces_lands_up_to_the_maximum(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity lamp = lamp_of(world, lamp_light());
	voe_scene_point_light three = lamp_light();
	voe_scene_point_light four = lamp_light();

	three.bounces = 3;
	four.bounces = 4;
	replace_and_run(world, lamp, three);
	check_light(voe_scene_point_light_get(world, lamp), three);
	replace_and_run(world, lamp, four);
	check_light(voe_scene_point_light_get(world, lamp), three);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_point_light_is(arena);
	a_refused_replace_keeps_the_row(arena);
	an_accepted_replace_lands_after_one_run(arena);
	a_falloff_at_either_bound_is_accepted(arena);
	cast_shadows_lands_either_way(arena);
	bounces_lands_up_to_the_maximum(arena);
	the_description_lists_the_bounce_last();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
