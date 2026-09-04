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
#include <base/arena.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	the_matrix_is_translate_rotate_scale();
	a_transform_round_trips_through_the_table(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	an_intent_for_a_destroyed_entity_is_dropped(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
