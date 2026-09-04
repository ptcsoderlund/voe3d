// The camera: that its numbers survive the table, that the view matrix puts the
// world where the camera can see it, and that every way of moving one behaves
// the way a hand expects.
//
// THESE WERE render/tests/camera.c UNTIL THE CAMERA MOVED HERE. Card 018 took
// the camera out of render, so the checks came with it — and they are the same
// claims, made against intents instead of against a function that took an input
// struct.
//
// EVERY CHECK NAMES A CONCRETE MISTAKE. A camera that turns the wrong way, one
// that sinks into the floor when it strafes, one that is faster diagonally, one
// that can look past straight up — all four draw a plausible picture and all
// four are wrong, so the checks below are about direction and amount and never
// about an identity.
#include <base/arena.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>

#include <testing/test.h>

#include <math.h>

#define TOLERANCE 1e-5f
#define CAMERAS 4

// A tenth of a second, so a metre a second is ten centimetres a step and the
// numbers below are readable.
#define DT 0.1f

// Enough steps that a wrong constant is obvious and few enough that the sums
// stay exact to the tolerance.
#define STEPS 10

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

static voe_scene_camera a_camera(void)
{
	voe_scene_camera camera = {
		.eye = { 0.0f, 0.0f, 0.0f },
		.yaw = 0.0f,
		.pitch = 0.0f,
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	return camera;
}

static voe_ecs_world *world_of(voe_base_arena *arena, voe_ecs_entity *eye)
{
	voe_ecs_limits limits = {
		.entities = CAMERAS,
		.component_types = 2,
		.intent_types = 3,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_camera_register(world, CAMERAS);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, eye));
	VOE_TEST_CHECK(voe_scene_camera_add(world, *eye, a_camera()));
	return world;
}

static voe_scene_camera camera_now(const voe_ecs_world *world,
				   voe_ecs_entity eye)
{
	const voe_scene_camera *row = voe_scene_camera_get(world, eye);

	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return a_camera();
	return *row;
}

// One step of being flown, submitted and drained, so every check goes through
// the same seam the frame loop does.
static voe_scene_camera stepped(voe_ecs_world *world, voe_ecs_entity eye,
				voe_scene_camera_motion motion)
{
	motion.entity = eye;
	motion.seconds = DT;
	VOE_TEST_CHECK(voe_scene_camera_move(world, motion));
	voe_scene_camera_system_run(world);
	return camera_now(world, eye);
}

static void the_numbers_survive_the_table(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera read = camera_now(world, eye);

	VOE_TEST_CHECK_FLOAT(read.fov_y, 1.0471976f, 0.0f);
	VOE_TEST_CHECK_FLOAT(read.near_plane, 0.1f, 0.0f);
	VOE_TEST_CHECK_FLOAT(read.far_plane, 100.0f, 0.0f);
	check_vector(read.eye, (voe_math_float3){ 0.0f, 0.0f, 0.0f });

	VOE_TEST_CHECK_INT(voe_scene_camera_count(world), 1);
	VOE_TEST_CHECK(voe_scene_camera_rows(world) ==
		       voe_scene_camera_get(world, eye));
	VOE_TEST_CHECK_INT(voe_scene_camera_entities(world)[0].index, eye.index);
}

// Zero yaw and zero pitch looks along -Z, so a camera at the origin has the
// identity for a view matrix and a thing five metres in front of it stays five
// metres in front of it.
static void a_camera_at_rest_looks_down_minus_z(void)
{
	voe_scene_camera camera = a_camera();
	voe_math_float4x4 view = voe_scene_camera_view(camera);
	voe_math_float3 ahead = { 0.0f, 0.0f, -5.0f };

	check_vector(voe_scene_camera_forward(camera),
		     (voe_math_float3){ 0.0f, 0.0f, -1.0f });
	check_vector(voe_math_float4x4_transform_point(view, ahead), ahead);

	// Stand it back along +Z and the world moves towards the camera by the
	// same amount, which is what a view matrix is.
	camera.eye = (voe_math_float3){ 0.0f, 0.0f, 5.0f };
	view = voe_scene_camera_view(camera);
	check_vector(voe_math_float4x4_transform_point(
			     view, (voe_math_float3){ 0.0f, 0.0f, 0.0f }),
		     (voe_math_float3){ 0.0f, 0.0f, -5.0f });
}

// A placement is absolute, and it applies before any motion in the same run —
// which is what makes handing a camera from a scripted path to a hand seamless
// instead of a jump.
static void a_placement_applies_before_a_motion(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera_placement placement = {
		.entity = eye,
		.eye = { 0.0f, 1.0f, 4.0f },
		.yaw = 0.0f,
		.pitch = 0.0f,
	};
	voe_scene_camera_motion motion = {
		.entity = eye,
		.forward = 1.0f,
		.seconds = DT,
	};
	voe_scene_camera read;

	// Submitted in the awkward order on purpose: the motion first, the
	// placement second, and the placement still wins.
	VOE_TEST_CHECK(voe_scene_camera_move(world, motion));
	VOE_TEST_CHECK(voe_scene_camera_place(world, placement));
	voe_scene_camera_system_run(world);

	read = camera_now(world, eye);
	// Placed at z = 4, then walked forward a tenth of a second at three
	// metres a second: 4 - 0.3.
	check_vector(read.eye, (voe_math_float3){ 0.0f, 1.0f, 3.7f });
}

// Moving the mouse right turns the camera right, and right is +X when it starts
// looking along -Z. Getting this backwards is a sign, and a sign is exactly the
// thing that looks fine in a screenshot.
static void the_mouse_turns_it_the_way_the_hand_went(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera read = stepped(world, eye,
					(voe_scene_camera_motion){
						.look_x = 100.0f });
	voe_math_float3 forward = voe_scene_camera_forward(read);

	VOE_TEST_CHECK(forward.x > 0.0f);
	VOE_TEST_CHECK_FLOAT(forward.y, 0.0f, TOLERANCE);

	// And down looks down.
	read = stepped(world, eye,
		       (voe_scene_camera_motion){ .look_y = 100.0f });
	VOE_TEST_CHECK(voe_scene_camera_forward(read).y < 0.0f);
}

// Look up as hard as anything can and it stops short of straight up, because at
// straight up the up vector the view matrix needs stops meaning anything.
static void pitch_stops_short_of_straight_up(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera read = a_camera();

	for (uint32_t i = 0; i < 100; i++)
		read = stepped(world, eye,
			       (voe_scene_camera_motion){
				       .look_y = -10000.0f });

	VOE_TEST_CHECK(read.pitch < 1.5707963f);
	VOE_TEST_CHECK(read.pitch > 1.5f);
	VOE_TEST_CHECK(voe_scene_camera_forward(read).y < 1.0f);

	for (uint32_t i = 0; i < 100; i++)
		read = stepped(world, eye,
			       (voe_scene_camera_motion){
				       .look_y = 10000.0f });

	VOE_TEST_CHECK(read.pitch > -1.5707963f);
	VOE_TEST_CHECK(read.pitch < -1.5f);
}

// Strafing while looking at the floor must not sink into it: right is taken
// horizontally rather than out of the camera's tilted frame.
static void strafing_stays_level_whatever_it_is_looking_at(
	voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera read;

	// Look well down first.
	for (uint32_t i = 0; i < 20; i++)
		(void)stepped(world, eye,
			      (voe_scene_camera_motion){ .look_y = 100.0f });

	read = camera_now(world, eye);
	VOE_TEST_CHECK(read.pitch < -0.5f);

	for (uint32_t i = 0; i < STEPS; i++)
		read = stepped(world, eye,
			       (voe_scene_camera_motion){ .right = 1.0f });

	VOE_TEST_CHECK_FLOAT(read.eye.y, 0.0f, TOLERANCE);
	VOE_TEST_CHECK(read.eye.x > 0.0f);

	// And up is world up, so rising while looking down still rises.
	for (uint32_t i = 0; i < STEPS; i++)
		read = stepped(world, eye,
			       (voe_scene_camera_motion){ .up = 1.0f });

	VOE_TEST_CHECK_FLOAT(read.eye.y, 3.0f, TOLERANCE);
}

// A diagonal is not faster than a straight line. Holding two keys asks for a
// direction, not for twice the speed, and the difference is whether the
// direction was normalized.
static void a_diagonal_is_not_faster(voe_base_arena *arena)
{
	voe_ecs_entity straight_eye = { 0 };
	voe_ecs_entity diagonal_eye = { 0 };
	voe_ecs_world *straight = world_of(arena, &straight_eye);
	voe_ecs_world *diagonal = world_of(arena, &diagonal_eye);
	voe_scene_camera one = a_camera();
	voe_scene_camera two = a_camera();
	float went_straight;
	float went_diagonally;

	for (uint32_t i = 0; i < STEPS; i++) {
		one = stepped(straight, straight_eye,
			      (voe_scene_camera_motion){ .forward = 1.0f });
		two = stepped(diagonal, diagonal_eye,
			      (voe_scene_camera_motion){ .forward = 1.0f,
							 .right = 1.0f });
	}

	went_straight = voe_math_float3_length(one.eye);
	went_diagonally = voe_math_float3_length(two.eye);

	VOE_TEST_CHECK_FLOAT(went_straight, 3.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(went_diagonally, went_straight, TOLERANCE);
}

// `fast` is a multiplier and holding it is four times as far in the same time.
static void fast_is_faster_by_exactly_the_multiplier(voe_base_arena *arena)
{
	voe_ecs_entity walking_eye = { 0 };
	voe_ecs_entity running_eye = { 0 };
	voe_ecs_world *walking = world_of(arena, &walking_eye);
	voe_ecs_world *running = world_of(arena, &running_eye);
	voe_scene_camera walked = a_camera();
	voe_scene_camera ran = a_camera();

	for (uint32_t i = 0; i < STEPS; i++) {
		walked = stepped(walking, walking_eye,
				 (voe_scene_camera_motion){ .forward = 1.0f });
		ran = stepped(running, running_eye,
			      (voe_scene_camera_motion){ .forward = 1.0f,
							 .fast = true });
	}

	VOE_TEST_CHECK_FLOAT(voe_math_float3_length(ran.eye),
			     voe_math_float3_length(walked.eye) * 4.0f,
			     TOLERANCE);
}

// Asking for nothing does nothing, which is the case a normalize would divide by
// zero on.
static void asking_for_nothing_moves_nothing(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);
	voe_scene_camera read = stepped(world, eye,
					(voe_scene_camera_motion){ 0 });

	check_vector(read.eye, (voe_math_float3){ 0.0f, 0.0f, 0.0f });
	VOE_TEST_CHECK_FLOAT(read.yaw, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(read.pitch, 0.0f, 0.0f);
}

// A camera destroyed between the submit and the drain takes its intents with it.
static void an_intent_for_a_destroyed_camera_is_dropped(voe_base_arena *arena)
{
	voe_ecs_entity eye = { 0 };
	voe_ecs_world *world = world_of(arena, &eye);

	VOE_TEST_CHECK(voe_scene_camera_move(
		world, (voe_scene_camera_motion){ .entity = eye,
						  .forward = 1.0f,
						  .seconds = DT }));
	voe_ecs_entity_destroy(world, eye);
	voe_scene_camera_system_run(world);

	VOE_TEST_CHECK(voe_scene_camera_get(world, eye) == NULL);
	VOE_TEST_CHECK_INT(voe_scene_camera_count(world), 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	the_numbers_survive_the_table(arena);
	a_camera_at_rest_looks_down_minus_z();
	a_placement_applies_before_a_motion(arena);
	the_mouse_turns_it_the_way_the_hand_went(arena);
	pitch_stops_short_of_straight_up(arena);
	strafing_stays_level_whatever_it_is_looking_at(arena);
	a_diagonal_is_not_faster(arena);
	fast_is_faster_by_exactly_the_multiplier(arena);
	asking_for_nothing_moves_nothing(arena);
	an_intent_for_a_destroyed_camera_is_dropped(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
