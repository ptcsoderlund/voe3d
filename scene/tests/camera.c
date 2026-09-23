// The camera: that registration says what a camera is to a tool, that the view
// is the inverse of the pose it is handed, and that the lens intent applies a
// good lens and keeps the row over a bad one.
//
// THE VIEW CHECKS NAME CONCRETE MISTAKES. A view that ignores the translation's
// sign, one that drops roll, and one that asserts on a pose scaled to nothing all
// draw something or stop the program; the three checks below catch each.
//
// THE DESCRIPTION IS SWITCHED ON HERE, WHATEVER THE BUILD SAID, for the reason
// scene/tests/transform.c gives at length. WHAT THE BUILD SAID IS KEPT FIRST,
// because the camera scene/src registers follows the build and not this file.
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
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define TOLERANCE 1e-5f
#define CAMERAS 4

static voe_scene_camera a_lens(void)
{
	return (voe_scene_camera){ .fov_y = 1.0471976f,
				   .near_plane = 0.1f,
				   .far_plane = 100.0f };
}

static voe_scene_transform unmoved(void)
{
	return (voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				      .scale = { 1.0f, 1.0f, 1.0f } };
}

static voe_ecs_world *world_of(voe_base_arena *arena, voe_ecs_entity *eye)
{
	voe_ecs_limits limits = {
		.entities = CAMERAS,
		.component_types = 2,
		.intent_types = 2,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, CAMERAS);
	voe_scene_camera_register(world, CAMERAS);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, eye));
	VOE_TEST_CHECK(voe_scene_transform_add(world, *eye, unmoved()));
	VOE_TEST_CHECK(voe_scene_camera_add(world, *eye, a_lens()));
	return world;
}

static void check_point(voe_math_float4x4 view, voe_math_float3 point,
			voe_math_float3 expected)
{
	voe_math_float3 seen = voe_math_float4x4_transform_point(view, point);

	VOE_TEST_CHECK_FLOAT(seen.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(seen.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(seen.z, expected.z, TOLERANCE);
}

static void check_lens(const voe_ecs_world *world, voe_ecs_entity eye,
		       voe_scene_camera expected)
{
	const voe_scene_camera *row = voe_scene_camera_get(world, eye);

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(row->fov_y, expected.fov_y, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->near_plane, expected.near_plane, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->far_plane, expected.far_plane, 0.0f);
}

static void check_field(const voe_base_field_description *actual,
			const char *name, size_t offset)
{
	VOE_TEST_CHECK(strcmp(actual->name, name) == 0);
	if (strcmp(actual->name, name) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual->name, name);
	VOE_TEST_CHECK_INT(actual->kind, VOE_BASE_FIELD_FLOAT32);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT((long long)actual->size, (long long)sizeof(float));
	VOE_TEST_CHECK_INT(actual->count, 1);
	VOE_TEST_CHECK(!actual->read_only);
}

// Three fields, the lens only, in the order declared and none read-only.
static void check_description(const voe_base_struct_description *description)
{
	const voe_base_field_description *fields = description->fields;

	VOE_TEST_CHECK(strcmp(description->name, "voe_scene_camera") == 0);
	VOE_TEST_CHECK_INT(description->field_count, 3);
	if (description->field_count != 3)
		return;
	check_field(&fields[0], "fov_y", offsetof(voe_scene_camera, fov_y));
	check_field(&fields[1], "near_plane",
		    offsetof(voe_scene_camera, near_plane));
	check_field(&fields[2], "far_plane",
		    offsetof(voe_scene_camera, far_plane));
}

// What a tool sees: the replace, the default row, the needed type, the field
// list, and no menu path, so Add component never offers a second camera.
static void registration_says_what_a_camera_is(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_ecs_type type = voe_ecs_component_type(world, &voe_scene_camera_key);
	voe_ecs_replace replace = voe_ecs_component_replace(world, type);
	const voe_scene_camera *row = voe_ecs_component_default(world, type);
	voe_ecs_type needed = { 0 };
	const voe_base_struct_description *found =
		voe_ecs_component_description(world, type);

	VOE_TEST_CHECK(replace.set);
	VOE_TEST_CHECK_INT((long long)replace.row_offset,
			   (long long)offsetof(voe_scene_camera_intent, camera));
	VOE_TEST_CHECK_INT((long long)replace.row_size,
			   (long long)sizeof(voe_scene_camera));
	VOE_TEST_CHECK_INT((long long)replace.value_size,
			   (long long)sizeof(voe_scene_camera_intent));

	VOE_TEST_CHECK(row != NULL);
	if (row != NULL) {
		VOE_TEST_CHECK_FLOAT(row->fov_y, 1.0471976f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->near_plane, 0.1f, 0.0f);
		VOE_TEST_CHECK_FLOAT(row->far_plane, 1000.0f, 0.0f);
	}

	VOE_TEST_CHECK(voe_ecs_component_needs(world, type, &needed));
	VOE_TEST_CHECK(voe_ecs_component_key(world, needed) ==
		       &voe_scene_transform_key);
	VOE_TEST_CHECK(voe_ecs_component_menu(world, type) == NULL);
	VOE_TEST_CHECK(!voe_ecs_component_runtime_only(world, type));

	check_description(voe_scene_camera_description());
	if (!BUILD_DESCRIBES) {
		VOE_TEST_CHECK(found == NULL);
		return;
	}
	VOE_TEST_CHECK(found != NULL);
	if (found != NULL)
		check_description(found);
}

// Five metres back along +Z, unrotated: the origin is five metres in front.
static void the_view_undoes_the_position(void)
{
	voe_scene_transform pose = unmoved();
	voe_math_float4x4 view;

	pose.position = (voe_math_float3){ 0.0f, 0.0f, 5.0f };
	VOE_TEST_CHECK(voe_scene_camera_view(pose, &view));
	check_point(view, (voe_math_float3){ 0.0f, 0.0f, 0.0f },
		    (voe_math_float3){ 0.0f, 0.0f, -5.0f });
}

// Rolled a quarter turn left about +Z: what is above the world's origin is to
// the camera's right, so a view that drops roll leaves it above.
static void the_view_keeps_the_roll(void)
{
	voe_scene_transform pose = unmoved();
	voe_math_float4x4 view;

	pose.rotation = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 0.0f, 1.0f }, 1.5707963f);
	VOE_TEST_CHECK(voe_scene_camera_view(pose, &view));
	check_point(view, (voe_math_float3){ 0.0f, 1.0f, -1.0f },
		    (voe_math_float3){ 1.0f, 0.0f, -1.0f });
}

static void a_pose_scaled_to_nothing_sees_nothing(void)
{
	voe_scene_transform pose = unmoved();
	voe_math_float4x4 view = { 0 };

	view.m[0][0] = 7.0f;
	pose.scale.y = 0.0f;
	VOE_TEST_CHECK(!voe_scene_camera_view(pose, &view));
	VOE_TEST_CHECK_FLOAT(view.m[0][0], 7.0f, 0.0f);
}

static void a_good_lens_applies(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera wider = { .fov_y = 1.5f,
				   .near_plane = 0.5f,
				   .far_plane = 50.0f };

	VOE_TEST_CHECK(voe_scene_camera_submit(
		world, (voe_scene_camera_intent){ .entity = eye,
						  .camera = wider }));
	check_lens(world, eye, a_lens());
	voe_scene_camera_system_run(world);
	check_lens(world, eye, wider);
}

// Each lens that cannot project, submitted alone: the row stays as it was.
static void a_bad_lens_leaves_the_row(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera bad[5];

	for (uint32_t i = 0; i < 5; i++)
		bad[i] = a_lens();
	bad[0].fov_y = 0.0f;
	bad[1].fov_y = 3.14159265f;
	bad[2].near_plane = 0.0f;
	bad[3].far_plane = bad[3].near_plane;
	bad[4].far_plane = NAN;

	for (uint32_t i = 0; i < 5; i++) {
		VOE_TEST_CHECK(voe_scene_camera_submit(
			world, (voe_scene_camera_intent){ .entity = eye,
							  .camera = bad[i] }));
		voe_scene_camera_system_run(world);
		check_lens(world, eye, a_lens());
	}
}

static void an_intent_for_a_destroyed_camera_is_dropped(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);

	VOE_TEST_CHECK(voe_scene_camera_submit(
		world, (voe_scene_camera_intent){ .entity = eye,
						  .camera = a_lens() }));
	voe_ecs_entity_destroy(world, eye);
	voe_scene_camera_system_run(world);

	VOE_TEST_CHECK(voe_scene_camera_get(world, eye) == NULL);
	VOE_TEST_CHECK_INT(voe_scene_camera_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	registration_says_what_a_camera_is(arena);
	the_view_undoes_the_position();
	the_view_keeps_the_roll();
	a_pose_scaled_to_nothing_sees_nothing();
	a_good_lens_applies(arena);
	a_bad_lens_leaves_the_row(arena);
	an_intent_for_a_destroyed_camera_is_dropped(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
