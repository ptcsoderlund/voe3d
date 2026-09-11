// The transform: that its matrix is translate·rotate·scale in that order, and
// that the only way to change one is an intent the system drains.
//
// THE ORDER IS THE CLAIM. All three of T·R·S, S·R·T and R·T·S place a thing
// somewhere plausible, and only one of them scales an object about its own
// centre and then puts it where it belongs. The check below picks a point, a
// rotation and a scale where each wrong order lands somewhere else entirely: a
// scale of two, a quarter turn about +Y and a translation with all three
// components different.
//
// A QUARTER TURN ABOUT +Y TAKES +Z TOWARDS +X (math/tests/quat.c), so it takes
// +X to -Z. That is where the -2 below comes from.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID. check.cmake builds
// without descriptions, and a check that followed the build would never run on
// the one run that gates a card. Nothing else in this file changes with it: the
// struct is the same either way.
//
// WHAT THE BUILD SAID IS KEPT FIRST, because the transform scene/src registers
// follows the build and not this file. The switch is set for a whole build, so
// with it off the world holds NULL for the transform, and that is checked too.
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
#define BUILD_DESCRIBES true
#else
#define BUILD_DESCRIBES false
#endif
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/arena.h>
#include <base/describe.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define QUARTER_TURN 1.5707963f

// Two trigonometric roundings and nine multiplies.
#define TOLERANCE 1e-5f

#define TRANSFORMS 8

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = TRANSFORMS,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, TRANSFORMS);
	return world;
}

static voe_scene_transform known(void)
{
	voe_math_float3 axis = { 0.0f, 1.0f, 0.0f };
	voe_scene_transform transform = {
		.position = { 1.0f, 2.0f, 3.0f },
		.rotation = voe_math_quat_from_axis_angle(axis, QUARTER_TURN),
		.scale = { 2.0f, 2.0f, 2.0f },
	};

	return transform;
}

static void the_matrix_is_translate_rotate_scale(void)
{
	voe_math_float4x4 m = voe_scene_transform_matrix(known());
	voe_math_float3 origin = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 along_x = { 1.0f, 0.0f, 0.0f };
	voe_math_float3 along_y = { 0.0f, 1.0f, 0.0f };

	// The object's own origin lands on the position, whatever the rotation
	// and the scale are. Any order that scaled or rotated the translation
	// would fail here first.
	check_vector(voe_math_float4x4_transform_point(m, origin),
		     (voe_math_float3){ 1.0f, 2.0f, 3.0f });

	// +X, scaled to two and then turned onto -Z, then moved.
	check_vector(voe_math_float4x4_transform_point(m, along_x),
		     (voe_math_float3){ 1.0f, 2.0f, 1.0f });

	// +Y is the rotation's axis, so it only scales and moves.
	check_vector(voe_math_float4x4_transform_point(m, along_y),
		     (voe_math_float3){ 1.0f, 4.0f, 3.0f });

	// The last column is the translation, which is what makes an upload to
	// the GPU a straight copy of this layout.
	VOE_TEST_CHECK_FLOAT(m.m[0][3], 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[1][3], 2.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[2][3], 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[3][3], 1.0f, 0.0f);
}

static void a_transform_round_trips_through_the_table(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_scene_transform given = known();
	voe_ecs_entity thing = { 0 };
	const voe_scene_transform *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_get(world, thing) == NULL);
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, given));

	read = voe_scene_transform_get(world, thing);
	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;

	check_vector(read->position, given.position);
	check_vector(read->scale, given.scale);
	VOE_TEST_CHECK_FLOAT(read->rotation.x, given.rotation.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->rotation.y, given.rotation.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->rotation.z, given.rotation.z, 0.0f);
	VOE_TEST_CHECK_FLOAT(read->rotation.w, given.rotation.w, 0.0f);

	VOE_TEST_CHECK_INT(voe_scene_transform_count(world), 1);
	VOE_TEST_CHECK(voe_scene_transform_rows(world) == read);
	VOE_TEST_CHECK_INT(voe_scene_transform_entities(world)[0].index,
			   thing.index);
}

// An intent changes nothing until the system runs, which is the whole of what
// rule 4 buys and the one thing a submitter could get wrong by assuming.
static void an_intent_lands_only_when_the_system_runs(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = { 0 };
	voe_scene_transform moved = known();
	const voe_scene_transform *read;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, known()));

	moved.position = (voe_math_float3){ 9.0f, 9.0f, 9.0f };
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = thing,
						     .transform = moved }));

	read = voe_scene_transform_get(world, thing);
	if (read != NULL)
		check_vector(read->position,
			     (voe_math_float3){ 1.0f, 2.0f, 3.0f });

	voe_scene_transform_system_run(world);

	read = voe_scene_transform_get(world, thing);
	if (read != NULL)
		check_vector(read->position,
			     (voe_math_float3){ 9.0f, 9.0f, 9.0f });

	// Two submitters, and the second one wins: an intent carries the whole
	// transform, so this is last-writer-wins rather than an accumulation in
	// an order nobody chose.
	moved.position = (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = thing,
						     .transform = moved }));
	moved.position = (voe_math_float3){ 2.0f, 0.0f, 0.0f };
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = thing,
						     .transform = moved }));
	voe_scene_transform_system_run(world);
	read = voe_scene_transform_get(world, thing);
	if (read != NULL)
		check_vector(read->position,
			     (voe_math_float3){ 2.0f, 0.0f, 0.0f });
}

// An entity that dies between the submit and the drain takes its intent with it,
// and that is ordinary rather than an error.
static void an_intent_for_a_destroyed_entity_is_dropped(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = { 0 };
	voe_ecs_entity other = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &other));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, known()));
	VOE_TEST_CHECK(voe_scene_transform_add(world, other, known()));

	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = thing,
						     .transform = known() }));
	voe_ecs_entity_destroy(world, thing);
	voe_scene_transform_system_run(world);

	VOE_TEST_CHECK(voe_scene_transform_get(world, thing) == NULL);
	VOE_TEST_CHECK_INT(voe_scene_transform_count(world), 1);
}

static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	if (strcmp(actual->name, name) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual->name, name);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT(actual->count, 1);
}

// Three fields, in the order they are declared, each kinded as declared and each
// at the offset the compiler gave it. A field list and a table that drifted apart
// would fail here rather than hand a reader the wrong bytes.
static void check_description(const voe_base_struct_description *description)
{
	const voe_base_field_description *fields = description->fields;

	VOE_TEST_CHECK(strcmp(description->name, "voe_scene_transform") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 3);
	if (description->field_count != 3)
		return;

	check_field(&fields[0], "position", VOE_BASE_FIELD_FLOAT3,
		    offsetof(voe_scene_transform, position));
	check_field(&fields[1], "rotation", VOE_BASE_FIELD_QUAT,
		    offsetof(voe_scene_transform, rotation));
	check_field(&fields[2], "scale", VOE_BASE_FIELD_FLOAT3,
		    offsetof(voe_scene_transform, scale));
}

static void the_description_is_the_struct_the_compiler_laid_out(void)
{
	check_description(voe_scene_transform_description());
}

// The inspector, minus the drawing: take an entity, ask the world what it is made
// of without naming a type, and reach the field list from the answer. The table
// the world holds is scene/src's own copy and not this file's (base/describe.h),
// so it is checked against the compiler field by field and never by address.
static void the_world_hands_back_the_transforms_field_list(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = { 0 };
	const voe_base_struct_description *found = NULL;
	uint32_t had = 0;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, known()));

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_get(world, type, thing) == NULL)
			continue;

		had++;
		found = voe_ecs_component_description(world, type);
		VOE_TEST_CHECK(voe_ecs_component_key(world, type) ==
			       &voe_scene_transform_key);
	}

	VOE_TEST_CHECK_INT(had, 1);

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

	the_matrix_is_translate_rotate_scale();
	the_description_is_the_struct_the_compiler_laid_out();
	the_world_hands_back_the_transforms_field_list(arena);
	a_transform_round_trips_through_the_table(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	an_intent_for_a_destroyed_entity_is_dropped(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
