// The sun: that a reader always gets a unit direction, and that turning it is an
// intent the system drains like everything else.
//
// THE NORMALIZATION IS THE CLAIM WORTH A TEST. Everything downstream of this
// component — the draw system, the object records, the shader's dot products —
// assumes the direction is unit length, and nothing on that path would fail
// loudly if it were not: a direction of length two makes a scene twice as bright
// and a direction of length a half makes it dim, and both look like somebody
// chose the wrong intensity. So both writes are checked, with a vector whose
// length is nowhere near one.
//
// (3, 4, 0) HAS LENGTH FIVE, which is why it is the vector below: every
// component of the answer is a fifth of what went in, and a normalization that
// silently did nothing leaves a 3 where a 0.6 belongs.
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
#include <scene/light_component.h>
#include <scene/light_system.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// One divide and three multiplies.
#define TOLERANCE 1e-6f

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

	voe_scene_light_register(world, LIGHTS);
	return world;
}

// Length five, pointing down and to the right, with a colour and an intensity
// that are neither white nor one — so that a field copied from the wrong place
// shows up as a wrong number rather than as the default.
static voe_scene_light known(void)
{
	voe_scene_light light = {
		.direction = { 3.0f, -4.0f, 0.0f },
		.colour = { 1.0f, 0.8f, 0.5f },
		.intensity = 2.5f,
	};

	return light;
}

static void a_light_arrives_normalized(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	const voe_scene_light *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_get(world, sun) == NULL);
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	read = voe_scene_light_get(world, sun);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;

	check_vector(read->direction,
		     (voe_math_float3){ 0.6f, -0.8f, 0.0f });
	VOE_TEST_CHECK_FLOAT(voe_math_float3_length(read->direction), 1.0f,
			     TOLERANCE);

	// Everything else is carried through untouched: only the direction is
	// this system's to change.
	check_vector(read->colour, known().colour);
	VOE_TEST_CHECK_FLOAT(read->intensity, known().intensity, 0.0f);

	VOE_TEST_CHECK_INT(voe_scene_light_count(world), 1);
	VOE_TEST_CHECK(voe_scene_light_rows(world) == read);
	VOE_TEST_CHECK_INT(voe_scene_light_entities(world)[0].index, sun.index);
}

// An intent changes nothing until the system runs, and what it does land is
// normalized as well — the second write into the table, and the one a frame
// loop uses every frame.
static void an_intent_lands_only_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	voe_scene_light turned = known();
	const voe_scene_light *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	turned.direction = (voe_math_float3){ 0.0f, 0.0f, -5.0f };
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = turned }));

	read = voe_scene_light_get(world, sun);
	if (read != NULL)
		check_vector(read->direction,
			     (voe_math_float3){ 0.6f, -0.8f, 0.0f });

	voe_scene_light_system_run(world);

	read = voe_scene_light_get(world, sun);
	if (read != NULL)
		check_vector(read->direction,
			     (voe_math_float3){ 0.0f, 0.0f, -1.0f });

	// Two submitters, and the second one wins: an intent carries the whole
	// light, so this is last-writer-wins and not an accumulation.
	turned.direction = (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = turned }));
	turned.direction = (voe_math_float3){ 0.0f, 2.0f, 0.0f };
	VOE_TEST_CHECK(voe_scene_light_submit(
		world,
		(voe_scene_light_intent){ .entity = sun, .light = turned }));
	voe_scene_light_system_run(world);
	read = voe_scene_light_get(world, sun);
	if (read != NULL)
		check_vector(read->direction,
			     (voe_math_float3){ 0.0f, 1.0f, 0.0f });
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

// Three fields, in the order they are declared, each kinded as declared and each
// at the offset and size the compiler gave it. None is read-only.
static void check_description(const voe_base_struct_description *description)
{
	const voe_base_field_description *fields = description->fields;

	VOE_TEST_CHECK(strcmp(description->name, "voe_scene_light") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 3);
	if (description->field_count != 3)
		return;

	check_field(&fields[0], "direction", VOE_BASE_FIELD_FLOAT3,
		    offsetof(voe_scene_light, direction), sizeof(voe_math_float3));
	check_field(&fields[1], "colour", VOE_BASE_FIELD_FLOAT3,
		    offsetof(voe_scene_light, colour), sizeof(voe_math_float3));
	check_field(&fields[2], "intensity", VOE_BASE_FIELD_FLOAT32,
		    offsetof(voe_scene_light, intensity), sizeof(float));
}

static void the_description_is_the_struct_the_compiler_laid_out(void)
{
	check_description(voe_scene_light_description());
}

// The inspector, minus the drawing: ask the world what an entity is made of
// without naming a type, and reach the field list from the answer. The table the
// world holds is scene/src's own copy, so it is checked field by field and never
// by address.
static void the_world_hands_back_the_lights_field_list(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity sun = { 0 };
	const voe_base_struct_description *found = NULL;
	bool runtime_only = true;
	uint32_t had = 0;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, known()));

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_get(world, type, sun) == NULL)
			continue;

		had++;
		found = voe_ecs_component_description(world, type);
		runtime_only = voe_ecs_component_runtime_only(world, type);
		VOE_TEST_CHECK(voe_ecs_component_key(world, type) ==
			       &voe_scene_light_key);
	}

	VOE_TEST_CHECK_INT(had, 1);

	// Not runtime-only in either build: with descriptions off a light is still
	// authored data, only without a table in this binary to show for it.
	VOE_TEST_CHECK(!runtime_only);

	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(found == NULL);
		return;
	}

	VOE_TEST_CHECK(found != NULL);
	if (found != NULL)
		check_description(found);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	a_light_arrives_normalized(arena);
	the_description_is_the_struct_the_compiler_laid_out();
	the_world_hands_back_the_lights_field_list(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	an_intent_for_a_destroyed_entity_is_dropped(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
