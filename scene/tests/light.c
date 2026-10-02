// The sun: what registration tells a tool, that an intent lands only when the
// system runs and a refused one keeps the row, and that a rotation and the
// direction it shines convert both ways. Bounces lands up to its maximum, 3, and
// is named "0" to "3"; bounce strength is 1 in the default and unsaid rows,
// listed last, and refused negative.
// Cast shadows is off by default, on in the unsaid row, and lands either way.
//
// THE CONVERSIONS ARE THE CLAIM WORTH MOST. Everything that draws the sun reads
// its direction through voe_scene_light_direction, and a sign wrong there lights
// exactly the faces that should be dark, which reads as a broken normal rather
// than a flipped light. So straight down is checked against a rotation built by
// hand, and facing is checked by turning a few directions into rotations and
// back, +Z (where every arc is as short) among them.
//
// EACH REFUSAL IS ITS OWN INTENT, submitted one at a time with the row checked
// after each, so a refusal that let one field through shows as that field.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/transform.c gives at length. WHAT THE BUILD SAID IS KEPT FIRST,
// because the light scene/src registers follows the build and not this file.
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
#define BUILD_DESCRIBES true
#else
#define BUILD_DESCRIBES false
#endif
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/quat.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// A quaternion built, turned into a direction and back.
#define TOLERANCE 1e-5f

#define LIGHTS 4

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = LIGHTS,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, LIGHTS);
	voe_scene_light_register(world, LIGHTS);
	return world;
}

// Every field neither white, one nor nought, so that a field copied from the
// wrong place shows up as a wrong number rather than as the default. Bounces
// takes neither nought nor one, and cast_shadows the value that is not the
// default.
static voe_scene_light known(void)
{
	return (voe_scene_light){
		.colour = { 1.0f, 0.8f, 0.5f },
		.intensity = 2.5f,
		.fill_colour = { 0.25f, 0.5f, 0.75f },
		.fill_intensity = 0.3f,
		.bounces = 2,
		.cast_shadows = true,
		.bounce_strength = 0.75f,
	};
}

static void check_light(const voe_scene_light *read, voe_scene_light expected)
{
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	check_vector(read->colour, expected.colour);
	VOE_TEST_CHECK_FLOAT(read->intensity, expected.intensity, 0.0f);
	check_vector(read->fill_colour, expected.fill_colour);
	VOE_TEST_CHECK_FLOAT(read->fill_intensity, expected.fill_intensity,
			     0.0f);
	VOE_TEST_CHECK_INT(read->bounces, expected.bounces);
	VOE_TEST_CHECK(read->cast_shadows == expected.cast_shadows);
	VOE_TEST_CHECK_FLOAT(read->bounce_strength, expected.bounce_strength,
			     0.0f);
}

static void a_light_arrives_as_given(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_get(world, sun) == NULL);
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	check_light(voe_scene_light_get(world, sun), known());
	VOE_TEST_CHECK_INT(voe_scene_light_count(world), 1);
	VOE_TEST_CHECK(voe_scene_light_rows(world) ==
		       voe_scene_light_get(world, sun));
	VOE_TEST_CHECK_INT(voe_scene_light_entities(world)[0].index, sun.index);
}

// An intent changes nothing until the system runs; two in one frame resolve as
// last-writer-wins, because an intent carries the whole light.
static void an_intent_lands_only_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_scene_light dimmer = known();
	voe_scene_light filled = known();

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	dimmer.intensity = 0.5f;
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = dimmer }));
	check_light(voe_scene_light_get(world, sun), known());
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), dimmer);

	filled.fill_intensity = 1.0f;
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = known() }));
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = filled }));
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), filled);
}

// Each bad number on its own: the row stays what it was.
static void a_refused_intent_keeps_the_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_scene_light bad[10];

	for (int i = 0; i < 10; i++)
		bad[i] = known();
	bad[0].colour.x = 1.5f;
	bad[1].colour.y = -0.1f;
	bad[2].intensity = -1.0f;
	bad[3].intensity = INFINITY;
	bad[4].fill_colour.z = NAN;
	bad[5].fill_colour.x = 2.0f;
	bad[6].fill_intensity = -0.5f;
	bad[7].bounces = VOE_SCENE_LIGHT_BOUNCES_MAX + 1;
	bad[8].bounce_strength = -1.0f;
	bad[9].bounce_strength = NAN;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	for (int i = 0; i < 10; i++) {
		VOE_TEST_CHECK(voe_scene_light_submit(
			world, (voe_scene_light_intent){ .entity = sun,
							 .light = bad[i] }));
		voe_scene_light_system_run(world);
		check_light(voe_scene_light_get(world, sun), known());
	}
}

// A light with none asks for three through an intent and gets them; asking for
// four, past the maximum, is refused and the three stay.
static void bounces_lands_up_to_the_maximum(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_scene_light none = known();
	voe_scene_light three = known();
	voe_scene_light four = known();

	none.bounces = 0;
	three.bounces = 3;
	four.bounces = 4;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, none));

	VOE_TEST_CHECK(voe_scene_light_submit(
		world, (voe_scene_light_intent){ .entity = sun, .light = three }));
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), three);

	VOE_TEST_CHECK(voe_scene_light_submit(
		world, (voe_scene_light_intent){ .entity = sun, .light = four }));
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), three);
}

// A light that casts nothing is switched on by an intent, and off by the next.
static void cast_shadows_lands_either_way(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_scene_light off = known();
	voe_scene_light on = known();

	off.cast_shadows = false;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, off));

	VOE_TEST_CHECK(voe_scene_light_submit(
		world, (voe_scene_light_intent){ .entity = sun, .light = on }));
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), on);

	VOE_TEST_CHECK(voe_scene_light_submit(
		world, (voe_scene_light_intent){ .entity = sun, .light = off }));
	voe_scene_light_system_run(world);
	check_light(voe_scene_light_get(world, sun), off);
}

// An entity that dies between the submit and the drain takes its intent with
// it, and that is ordinary rather than an error.
static void an_intent_for_a_destroyed_entity_is_dropped(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_ecs_entity other = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &other));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));
	VOE_TEST_CHECK(voe_scene_light_add(world, other, known()));

	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = known() }));
	voe_ecs_entity_destroy(world, sun);
	voe_scene_light_system_run(world);

	VOE_TEST_CHECK(voe_scene_light_get(world, sun) == NULL);
	VOE_TEST_CHECK_INT(voe_scene_light_count(world), 1);
}

static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset, size_t size)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	if (strcmp(actual->name, name) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual->name, name);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT((long long)actual->size, (long long)size);
	VOE_TEST_CHECK_INT(actual->count, 1);
	VOE_TEST_CHECK(!actual->read_only);
}

// The one named field: bounces, its counts named "0" to "3".
static void check_bounces_names(const voe_base_struct_description *description)
{
	const voe_base_field_names *names =
		voe_base_names_find(description, "bounces");
	const char *const expected[] = { "0", "1", "2", "3" };

	VOE_TEST_CHECK_INT(description->names_count, 1);
	VOE_TEST_CHECK(names != NULL);
	if (names == NULL)
		return;
	VOE_TEST_CHECK_INT(names->value_count, 4);
	if (names->value_count != 4)
		return;
	for (int i = 0; i < 4; i++)
		VOE_TEST_CHECK(strcmp(names->values[i], expected[i]) == 0);
}

// Seven fields, in the order they are declared, each kinded as declared and
// each at the offset and size the compiler gave it. None is read-only.
static void check_description(const voe_base_struct_description *description)
{
	const voe_base_field_description *fields = description->fields;

	VOE_TEST_CHECK(strcmp(description->name, "voe_scene_light") == 0);
	check_bounces_names(description);
	VOE_TEST_CHECK_INT(description->field_count, 7);
	if (description->field_count != 7)
		return;

	check_field(&fields[0], "colour", VOE_BASE_FIELD_COLOUR,
		    offsetof(voe_scene_light, colour), sizeof(voe_math_float3));
	check_field(&fields[1], "intensity", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_light, intensity), sizeof(float));
	check_field(&fields[2], "fill_colour", VOE_BASE_FIELD_COLOUR,
		    offsetof(voe_scene_light, fill_colour),
		    sizeof(voe_math_float3));
	check_field(&fields[3], "fill_intensity", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_light, fill_intensity), sizeof(float));
	check_field(&fields[4], "bounces", VOE_BASE_FIELD_UINT32,
		    offsetof(voe_scene_light, bounces), sizeof(uint32_t));
	check_field(&fields[5], "cast_shadows", VOE_BASE_FIELD_BOOL,
		    offsetof(voe_scene_light, cast_shadows), sizeof(bool));
	check_field(&fields[6], "bounce_strength", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_light, bounce_strength), sizeof(float));
}

// What a tool sees: the replace, the default row (0190: white of strength one,
// a white fill of nought, no bounces at a bounce strength of one, no shadows),
// the unsaid row (0324: the
// default casting), the transform it needs, the menu path and the field list,
// which in a describing build is scene/src's own copy and so is checked field
// by field and never by address.
static void registration_says_what_a_light_is(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_light_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	const voe_scene_light *row = voe_ecs_component_default(world, type);
	const voe_scene_light *unsaid = voe_ecs_component_unsaid(world, type);
	voe_scene_light expected = { .colour = { 1.0f, 1.0f, 1.0f },
				     .intensity = 1.0f,
				     .fill_colour = { 1.0f, 1.0f, 1.0f },
				     .fill_intensity = 0.0f,
				     .bounces = 0,
				     .cast_shadows = false,
				     .bounce_strength = 1.0f };
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *found =
		voe_ecs_component_description(world, type);

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_light_intent, light));
	VOE_TEST_CHECK_INT((long long)replace.row_size,
			   (long long)sizeof(voe_scene_light));
	VOE_TEST_CHECK_INT((long long)replace.value_size,
			   (long long)sizeof(voe_scene_light_intent));

	check_light(row, expected);
	expected.cast_shadows = true;
	check_light(unsaid, expected);

	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(strcmp(voe_ecs_component_menu(world, type),
			      "Rendering / Light") == 0);
	// Authored in either build, with or without a table to show for it.
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	check_description(voe_scene_light_description());
	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(found == NULL);
		return;
	}
	VOE_TEST_CHECK(found != NULL);
	if (found != NULL)
		check_description(found);
}

// The unturned light shines along -Z; a quarter turn of -90 degrees about X
// takes -Z to straight down, and a non-unit rotation gives the same.
static void a_rotation_becomes_the_direction_it_shines(void)
{
	voe_math_quat down = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 1.0f, 0.0f, 0.0f }, -1.5707963f);

	check_vector(voe_scene_light_direction(
			     (voe_math_quat){ 0.0f, 0.0f, 0.0f, 1.0f }),
		     (voe_math_float3){ 0.0f, 0.0f, -1.0f });
	check_vector(voe_scene_light_direction(down),
		     (voe_math_float3){ 0.0f, -1.0f, 0.0f });
	check_vector(voe_scene_light_direction((voe_math_quat){
			     down.x * 3.0f, down.y * 3.0f, down.z * 3.0f,
			     down.w * 3.0f }),
		     (voe_math_float3){ 0.0f, -1.0f, 0.0f });
}

// Facing and back gives the unit direction facing was handed, whatever its
// length; +Z is a half turn about Y.
static void a_direction_becomes_a_rotation_and_back(void)
{
	const voe_math_float3 directions[] = {
		{ 0.0f, -1.0f, 0.0f }, { 3.0f, -4.0f, 0.0f },
		{ -0.4f, -1.0f, -0.3f }, { 0.0f, 0.0f, -2.0f },
		{ 1.0f, 0.0f, 0.001f }, { 0.2f, 0.1f, 5.0f },
	};
	voe_math_quat half = voe_scene_light_facing(
		(voe_math_float3){ 0.0f, 0.0f, 3.0f });

	for (size_t i = 0; i < sizeof directions / sizeof directions[0]; i++)
		check_vector(voe_scene_light_direction(
				     voe_scene_light_facing(directions[i])),
			     voe_math_float3_normalize(directions[i]));

	VOE_TEST_CHECK_FLOAT(half.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(half.y, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(half.z, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(half.w, 0.0f, 0.0f);
	check_vector(voe_scene_light_direction(half),
		     (voe_math_float3){ 0.0f, 0.0f, 1.0f });
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	registration_says_what_a_light_is(arena);
	a_light_arrives_as_given(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	a_refused_intent_keeps_the_row(arena);
	bounces_lands_up_to_the_maximum(arena);
	cast_shadows_lands_either_way(arena);
	an_intent_for_a_destroyed_entity_is_dropped(arena);
	a_rotation_becomes_the_direction_it_shines();
	a_direction_becomes_a_rotation_and_back();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
