// The transform: that its matrix is translate·rotate·scale in that order, and
// that the only way to change one is an intent the system drains.
//
// THE DRAIN IS THE OTHER CLAIM, AND THE RAW SUBMIT IS HOW IT IS REACHED. Neither
// typed call checks a rotation, so both doors reach the same settling — which is
// itself one of the checks below. What reaches the drain in a running program is
// whatever a tool that knows only the offsets put in the queue (ecs/component.h),
// so half of these submit through voe_ecs_intent_submit with the bytes such a
// tool would write.
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
// with it off the world hands back NULL for the transform, and that is checked
// too — along with the type still not being runtime-only.
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
#include <ecs/intent.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
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

// The bytes a tool that knows only the offsets would write, straight into the
// queue the world names as the transform's replace. This is the door the drain's
// settling has to hold whether or not the typed call was used.
static bool submit_raw(voe_ecs_world *world, voe_ecs_entity entity,
		       voe_scene_transform transform)
{
	voe_scene_transform_intent intent = { .entity = entity,
					      .transform = transform };
	voe_ecs_type type = voe_ecs_component_type(world,
						   &voe_scene_transform_key);
	voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;

	return voe_ecs_intent_submit(world, queue, &intent);
}

// An entity already holding the known transform, ready to have an intent aimed
// at it — and so also the row that is kept when one is refused.
static voe_ecs_entity placed(voe_ecs_world *world)
{
	voe_ecs_entity thing = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, known()));
	return thing;
}

static voe_math_quat lengthened(voe_math_quat q, float by)
{
	return (voe_math_quat){ q.x * by, q.y * by, q.z * by, q.w * by };
}

static void check_rotation(voe_math_quat actual, voe_math_quat expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.w, expected.w, TOLERANCE);
}

// The row as it stands, for a check that wants to prove nothing moved.
static voe_scene_transform row_of(const voe_ecs_world *world,
				  voe_ecs_entity entity)
{
	const voe_scene_transform *read = voe_scene_transform_get(world, entity);

	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return (voe_scene_transform){ 0 };

	return *read;
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

// The door an inspector knocks on: the world says which intent replaces a whole
// transform row, and where in that intent the row sits.
static void the_world_names_the_intent_that_replaces_a_transform(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_type type = voe_ecs_component_type(world,
						   &voe_scene_transform_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT(
		(long long)replace.row_offset,
		(long long)offsetof(voe_scene_transform_intent, transform));
	VOE_TEST_CHECK_INT((long long)replace.row_size,
			   (long long)sizeof(voe_scene_transform));
	VOE_TEST_CHECK_INT((long long)replace.value_size,
			   (long long)sizeof(voe_scene_transform_intent));
}

// A rotation twice as long as a rotation is the same rotation, and that is what
// makes normalising it a correction rather than a guess: the direction survives
// and only the length was ever wrong. The one a fifth of a per cent out is the
// silent half of the same rule.
static void a_rotation_arrives_unit_length(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = placed(world);
	voe_scene_transform stretched = known();
	voe_scene_transform drifted = known();

	stretched.rotation = lengthened(known().rotation, 2.0f);
	VOE_TEST_CHECK(submit_raw(world, thing, stretched));
	voe_scene_transform_system_run(world);

	VOE_TEST_CHECK_FLOAT(
		voe_math_quat_length(row_of(world, thing).rotation), 1.0f,
		TOLERANCE);
	check_rotation(row_of(world, thing).rotation, known().rotation);

	// And the position beside it landed, because this was a correction to
	// one field and not a refusal of the row.
	stretched.position = (voe_math_float3){ 4.0f, 5.0f, 6.0f };
	stretched.rotation = lengthened(known().rotation, 2.0f);
	VOE_TEST_CHECK(submit_raw(world, thing, stretched));
	voe_scene_transform_system_run(world);
	check_vector(row_of(world, thing).position,
		     (voe_math_float3){ 4.0f, 5.0f, 6.0f });

	drifted.rotation = lengthened(known().rotation, 1.0005f);
	VOE_TEST_CHECK(submit_raw(world, thing, drifted));
	voe_scene_transform_system_run(world);

	VOE_TEST_CHECK_FLOAT(
		voe_math_quat_length(row_of(world, thing).rotation), 1.0f,
		TOLERANCE);
	check_rotation(row_of(world, thing).rotation, known().rotation);
}

// Neither of these has a nearest rotation to be corrected towards, so the row
// the entity already had is what it still has: settling refuses rather than
// invents.
static void a_row_with_no_valid_value_is_left_alone(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity thing = placed(world);
	voe_scene_transform zeroed = known();
	voe_scene_transform not_a_number = known();

	zeroed.rotation = (voe_math_quat){ 0.0f, 0.0f, 0.0f, 0.0f };
	zeroed.position = (voe_math_float3){ 7.0f, 7.0f, 7.0f };
	VOE_TEST_CHECK(submit_raw(world, thing, zeroed));
	voe_scene_transform_system_run(world);

	// The position it came with is refused with it: the whole row is one
	// intent, and half of a refused one is not an edit anybody submitted.
	check_vector(row_of(world, thing).position,
		     (voe_math_float3){ 1.0f, 2.0f, 3.0f });
	check_rotation(row_of(world, thing).rotation, known().rotation);

	not_a_number.position = (voe_math_float3){ NAN, 2.0f, 3.0f };
	VOE_TEST_CHECK(submit_raw(world, thing, not_a_number));
	voe_scene_transform_system_run(world);

	check_vector(row_of(world, thing).position,
		     (voe_math_float3){ 1.0f, 2.0f, 3.0f });
	check_rotation(row_of(world, thing).rotation, known().rotation);
}

// The claim the typed call exists to not be special about: an intent through it
// and one written straight into the queue are settled by the same code, so the
// importer and a tool that knows only the offsets get the same answer.
static void both_doors_are_settled_the_same_way(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity typed = placed(world);
	voe_ecs_entity raw = placed(world);
	voe_scene_transform stretched = known();

	stretched.rotation = lengthened(known().rotation, 2.0f);

	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){
			       .entity = typed, .transform = stretched }));
	VOE_TEST_CHECK(submit_raw(world, raw, stretched));
	voe_scene_transform_system_run(world);

	check_rotation(row_of(world, typed).rotation,
		       row_of(world, raw).rotation);
	check_rotation(row_of(world, typed).rotation, known().rotation);
}

// A drain with nothing in it, which is what closes a run of settlings and writes
// its count. Between the checks that settle something so that each of them
// starts a run of its own, and every line the system can write is written once in
// a passing run — the format is for a person reading stderr, and a format nothing
// ever prints is a format nobody has read.
static void a_quiet_drain(voe_base_arena *arena)
{
	voe_scene_transform_system_run(world_of(arena));
}

// The report names the entity by its identity where the world has identities, and
// by index and generation where it has none — every other check in this file uses
// a world with no identity table, which is the branch dev takes. This one has
// both, so the naming walk runs on a world where it finds something.
static void a_named_entity_is_reported_by_name(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = TRANSFORMS,
		.component_types = 3,
		.intent_types = 3,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity named = { 0 };
	voe_ecs_entity anonymous;
	voe_scene_identity identity = { .id = 2 };
	voe_scene_transform stretched = known();

	voe_scene_transform_register(world, TRANSFORMS);
	voe_scene_identity_register(world, TRANSFORMS);

	memcpy(identity.name, "Cube_2", strlen("Cube_2"));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &named));
	VOE_TEST_CHECK(voe_scene_transform_add(world, named, known()));
	VOE_TEST_CHECK(voe_scene_identity_add(world, named, identity));
	anonymous = placed(world);

	stretched.rotation = lengthened(known().rotation, 2.0f);
	VOE_TEST_CHECK(submit_raw(world, named, stretched));
	VOE_TEST_CHECK(submit_raw(world, anonymous, stretched));
	voe_scene_transform_system_run(world);

	// The line itself is for a person; what is checked is that the walk that
	// wrote it did not disturb what it was reporting on.
	check_rotation(row_of(world, named).rotation, known().rotation);
	check_rotation(row_of(world, anonymous).rotation, known().rotation);
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
	bool runtime_only = true;
	uint32_t had = 0;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, known()));

	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_get(world, type, thing) == NULL)
			continue;

		had++;
		found = voe_ecs_component_description(world, type);
		runtime_only = voe_ecs_component_runtime_only(world, type);
		VOE_TEST_CHECK(voe_ecs_component_key(world, type) ==
			       &voe_scene_transform_key);
	}

	VOE_TEST_CHECK_INT(had, 1);

	// Not runtime-only in either build: with descriptions off the type is
	// still authored data, and a NULL here that also said runtime-only would
	// be a scene that saves nothing and says nothing.
	VOE_TEST_CHECK(!runtime_only);

	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(found == NULL);
		return;
	}

	VOE_TEST_CHECK(found != NULL);
	if (found != NULL)
		check_description(found);
}

// What "add at default" gives (ADR-0190): at the origin, unrotated, scale one.
// A zeroed rotation or scale would be a transform that draws nothing.
static void the_default_is_the_origin_unrotated_at_scale_one(
	voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	const voe_scene_transform *row = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(row->position.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->position.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->position.z, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->rotation.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->rotation.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->rotation.z, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->rotation.w, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->scale.x, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->scale.y, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->scale.z, 1.0f, 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	the_matrix_is_translate_rotate_scale();
	the_description_is_the_struct_the_compiler_laid_out();
	the_world_hands_back_the_transforms_field_list(arena);
	the_world_names_the_intent_that_replaces_a_transform(arena);
	the_default_is_the_origin_unrotated_at_scale_one(arena);
	a_transform_round_trips_through_the_table(arena);
	an_intent_lands_only_when_the_system_runs(arena);
	an_intent_for_a_destroyed_entity_is_dropped(arena);

	a_rotation_arrives_unit_length(arena);
	a_quiet_drain(arena);
	a_row_with_no_valid_value_is_left_alone(arena);
	a_quiet_drain(arena);
	both_doors_are_settled_the_same_way(arena);
	a_quiet_drain(arena);
	a_named_entity_is_reported_by_name(arena);
	a_quiet_drain(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
