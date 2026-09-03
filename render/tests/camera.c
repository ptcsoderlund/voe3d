// THE FLOWN CAMERA, ON THE CPU, AND NOT ONE GRAPHICS CARD IS NEEDED FOR ANY OF
// IT. voe_render_cube_camera_step is arithmetic over five floats and a bool, so
// every claim here is a call and a comparison. That matters more than usual for
// this one: the camera is flown with a keyboard and a mouse, the person holding
// them is on one operating system at a time, and "it felt right on Linux" is not
// a statement about Windows. What is checked here is the half that cannot differ
// between the two.
//
// WHAT IS NOT CHECKED HERE, AND IT IS THE OTHER HALF. Whether a key press
// reaches this function at all is platform's, and it is two different pieces of
// code on the two platforms — a wl_keyboard listener and a window procedure.
// Nothing in this file can see either. So this file says the camera is right and
// says nothing about the keyboard; the keyboard is looked at, by a person, on
// each platform, and dev/src/main.c's header is the list of what to look for.
//
// EVERY CLAIM IS ONE OF THE WAYS THIS KIND OF CAMERA GOES WRONG, and each of
// them is a specific mistake rather than a general "it works":
//
//   - The handover from the orbit does not move the picture. Seeding a position
//     and forgetting the angles is the usual version of this and it reads as a
//     jump the moment a person takes control.
//   - Pitch stops short of straight up. Past it the look-at's up vector and its
//     direction of view are parallel, the cross product is zero, and the view
//     matrix fills with NaN — the whole frame disappears and nothing says why.
//   - A diagonal is not faster than a straight line. Two keys held give a
//     direction of length root two, and moving along it unscaled is forty per
//     cent quicker on the diagonal. It is invisible until somebody races along
//     one, and it is the oldest bug there is in a camera like this.
//   - Strafing stays horizontal while looking at the floor. Taking `right` from
//     the camera's own frame instead of keeping it level sinks the camera into
//     the ground when it strafes while looking down.
//   - The mouse turns the camera the way the mouse went. A sign here is a
//     one-character mistake that nothing else can catch.
//   - Not flying stores nothing. The orbit is a function of the clock, and a
//     camera that remembered anything while nobody was flying it would make the
//     orbit depend on what happened before it.
//
// IT INCLUDES render's INTERNAL HEADER BY RELATIVE PATH, exactly as the three
// other tests in this folder do and for the same reason: the camera and the
// function that steps it are not render's public surface and must not become
// part of it so that a test can see them.
#include "../src/device_internal.h"

#include <testing/test.h>

#include <math.h>

// A frame's worth of time. Sixty per second, which is what frame.c counts, and
// it is a round number here so that a step at one metre per second is a
// sixtieth of a metre and the arithmetic can be read.
#define DT (1.0f / 60.0f)

// Distances are in metres and a millimetre is far tighter than anything here
// claims. Angles are in radians and this is a thousandth of a degree.
#define METRE_TOLERANCE 0.001f
#define RADIAN_TOLERANCE 0.00002f

// The direction the view matrix says the camera is looking, recovered from the
// matrix rather than from the camera's own angles — so that a claim about where
// it looks is a claim about what the renderer will actually use.
//
// The third row of a view matrix is the camera's own +Z in world space, which
// points back at the viewer, so the direction of view is its negative. Every
// claim below about "looking at" goes through here for that reason: an angle
// that was stored correctly and then built into the wrong matrix would pass a
// test that read the angle.
static voe_math_float3 view_direction(voe_math_float4x4 view)
{
	return (voe_math_float3){ -view.m[2][0], -view.m[2][1],
				  -view.m[2][2] };
}

// One step of nothing at all, which is what a caller that is flying but not
// pressing anything sends. Used wherever a claim needs the camera to be flying
// without also moving.
static voe_render_camera_input flying_still(void)
{
	return (voe_render_camera_input){ .fly = true };
}

// ------------------------------------------------------ the handover

// TAKING CONTROL DOES NOT MOVE THE PICTURE, AND THAT IS TWO CLAIMS AND NOT ONE:
// the camera is where the orbit was, and it is looking where the orbit looked.
// A version that seeds the position and leaves the angles at zero passes the
// first and fails the second, which is exactly the bug this is here for.
//
// Checked at several moments on the orbit rather than at one, because at zero
// seconds the orbit happens to sit on the +Z axis with a yaw of nought — the one
// angle a camera that forgot to seed its yaw would also have.
static void handover_does_not_jump(void)
{
	VkExtent2D extent = { 640, 360 };
	static const float MOMENTS[] = { 0.0f, 1.0f, 3.0f, 6.0f, 9.5f };

	for (unsigned moment = 0; moment < sizeof(MOMENTS) / sizeof(MOMENTS[0]);
	     moment++) {
		float seconds = MOMENTS[moment];
		struct voe_render_camera camera = { 0 };
		struct voe_render_uniforms orbiting;
		struct voe_render_uniforms flown;

		// Where the orbit is at this moment, through the same function
		// the renderer calls.
		voe_render_cube_uniforms_fill(&orbiting, extent, &camera,
					      seconds);

		// The frame control is taken, with nothing else asked for.
		voe_render_cube_camera_step(&camera, flying_still(), seconds,
					    DT);
		VOE_TEST_CHECK(camera.flying);

		voe_render_cube_uniforms_fill(&flown, extent, &camera, seconds);

		// THE SAME VIEW MATRIX, ELEMENT FOR ELEMENT. Both halves of the
		// handover are in this one comparison: a position that was not
		// carried over moves the translation column, and an angle that
		// was not carried over moves the rotation rows.
		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++)
				VOE_TEST_CHECK_FLOAT(flown.view.m[row][column],
						     orbiting.view.m[row][column],
						     METRE_TOLERANCE);
		}
	}
}

// NOT FLYING STORES NOTHING, which is what keeps the orbit a function of the
// clock alone. Flying about for a while, handing control back, and then asking
// for the orbit must give the orbit — not the orbit plus wherever the camera
// happened to end up.
static void handing_back_keeps_nothing(void)
{
	VkExtent2D extent = { 640, 360 };
	struct voe_render_camera camera = { 0 };
	struct voe_render_camera untouched = { 0 };
	voe_render_camera_input away = { .fly = true, .forward = 1.0f };
	struct voe_render_uniforms flown_then_released;
	struct voe_render_uniforms never_flown;
	voe_math_float3 start;

	// Fly forward for a hundred frames, which is metres away from where the
	// orbit had the camera. Measured from where it started and not from the
	// origin: forward at the handover points *at* the origin, so a camera
	// flying that way passes through it and comes out no further from it
	// than it went in — which says nothing about how far it went.
	voe_render_cube_camera_step(&camera, away, 1.0f, DT);
	start = camera.eye;
	for (int frame = 0; frame < 100; frame++)
		voe_render_cube_camera_step(&camera, away, 1.0f, DT);
	VOE_TEST_CHECK(voe_math_float3_length(
			       voe_math_float3_sub(camera.eye, start)) > 4.0f);

	// Hand it back.
	voe_render_cube_camera_step(&camera, (voe_render_camera_input){ 0 },
				    1.0f, DT);
	VOE_TEST_CHECK(!camera.flying);

	voe_render_cube_uniforms_fill(&flown_then_released, extent, &camera,
				      1.0f);
	voe_render_cube_uniforms_fill(&never_flown, extent, &untouched, 1.0f);

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			VOE_TEST_CHECK_FLOAT(
				flown_then_released.view.m[row][column],
				never_flown.view.m[row][column],
				METRE_TOLERANCE);
	}
}

// ------------------------------------------------------------ looking

// THE MOUSE TURNS THE CAMERA THE WAY THE MOUSE WENT, and all four directions are
// checked because a sign is a one-character mistake.
//
// platform reports +x to the right and +y downwards, so: right takes the
// direction of view towards the camera's own right, which at a starting yaw of
// nought is +X; down takes it downwards, which is -Y. Both are read out of the
// view matrix rather than out of the camera's angles, for the reason
// view_direction says.
static void mouse_turns_the_right_way(void)
{
	VkExtent2D extent = { 640, 360 };
	static const struct {
		const char *what;
		float look_x;
		float look_y;
	} MOVES[] = {
		{ "right", 200.0f, 0.0f },
		{ "left", -200.0f, 0.0f },
		{ "down", 0.0f, 200.0f },
		{ "up", 0.0f, -200.0f },
	};

	for (unsigned move = 0; move < sizeof(MOVES) / sizeof(MOVES[0]);
	     move++) {
		struct voe_render_camera camera = { 0 };
		voe_render_camera_input look = flying_still();
		struct voe_render_uniforms uniforms;
		voe_math_float3 before;
		voe_math_float3 after;

		// Take control at zero seconds, where the orbit's yaw is
		// nought: the camera looks along -Z, its right is +X and its up
		// is +Y, so each claim below is about one axis and not two.
		voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);
		camera.pitch = 0.0f;
		camera.yaw = 0.0f;
		voe_render_cube_uniforms_fill(&uniforms, extent, &camera, 0.0f);
		before = view_direction(uniforms.view);
		VOE_TEST_CHECK_FLOAT(before.x, 0.0f, RADIAN_TOLERANCE);
		VOE_TEST_CHECK_FLOAT(before.y, 0.0f, RADIAN_TOLERANCE);
		VOE_TEST_CHECK_FLOAT(before.z, -1.0f, RADIAN_TOLERANCE);

		look.look_x = MOVES[move].look_x;
		look.look_y = MOVES[move].look_y;
		voe_render_cube_camera_step(&camera, look, 0.0f, DT);
		voe_render_cube_uniforms_fill(&uniforms, extent, &camera, 0.0f);
		after = view_direction(uniforms.view);

		// Mouse right means the view goes right, which is +X. Left is
		// -X. Neither should touch the height it is looking at.
		if (MOVES[move].look_x > 0.0f) {
			VOE_TEST_CHECK(after.x > 0.0f);
			VOE_TEST_CHECK_FLOAT(after.y, 0.0f, RADIAN_TOLERANCE);
		} else if (MOVES[move].look_x < 0.0f) {
			VOE_TEST_CHECK(after.x < 0.0f);
			VOE_TEST_CHECK_FLOAT(after.y, 0.0f, RADIAN_TOLERANCE);
		}

		// Mouse down means the view goes down, which is -Y. Up is +Y.
		// Neither should turn it sideways.
		if (MOVES[move].look_y > 0.0f) {
			VOE_TEST_CHECK(after.y < 0.0f);
			VOE_TEST_CHECK_FLOAT(after.x, 0.0f, RADIAN_TOLERANCE);
		} else if (MOVES[move].look_y < 0.0f) {
			VOE_TEST_CHECK(after.y > 0.0f);
			VOE_TEST_CHECK_FLOAT(after.x, 0.0f, RADIAN_TOLERANCE);
		}

		// And it is still a direction. A pair of angles turned into a
		// vector that is not unit is a projection that scales with
		// where the camera is looking.
		VOE_TEST_CHECK_FLOAT(voe_math_float3_length(after), 1.0f,
				     RADIAN_TOLERANCE);
	}
}

// PITCH STOPS SHORT OF STRAIGHT UP AND OF STRAIGHT DOWN, AND THE VIEW MATRIX
// STAYS A NUMBER. Ten thousand units of mouse in one direction is far past any
// limit; what must not happen is a view matrix with a NaN in it, which is what
// an unclamped pitch produces the moment the direction of view and the world's
// up are parallel.
static void pitch_is_clamped(void)
{
	VkExtent2D extent = { 640, 360 };
	static const float DIRECTIONS[] = { 10000.0f, -10000.0f };

	for (unsigned which = 0;
	     which < sizeof(DIRECTIONS) / sizeof(DIRECTIONS[0]); which++) {
		struct voe_render_camera camera = { 0 };
		voe_render_camera_input look = flying_still();
		struct voe_render_uniforms uniforms;
		voe_math_float3 direction;

		voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);

		// Twenty frames of a mouse being dragged as far as it will go.
		look.look_y = DIRECTIONS[which];
		for (int frame = 0; frame < 20; frame++)
			voe_render_cube_camera_step(&camera, look, 0.0f, DT);

		// Short of a quarter turn, on the side it was pushed towards.
		VOE_TEST_CHECK(fabsf(camera.pitch) < 1.5707963f);
		if (DIRECTIONS[which] > 0.0f)
			VOE_TEST_CHECK(camera.pitch < 0.0f);
		else
			VOE_TEST_CHECK(camera.pitch > 0.0f);

		// THE POINT OF THE CLAMP: THE MATRIX IS STILL A MATRIX. Every
		// element compared with itself, which is the one comparison a
		// NaN fails.
		voe_render_cube_uniforms_fill(&uniforms, extent, &camera, 0.0f);
		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++)
				VOE_TEST_CHECK(uniforms.view.m[row][column] ==
					       uniforms.view.m[row][column]);
		}

		direction = view_direction(uniforms.view);
		VOE_TEST_CHECK_FLOAT(voe_math_float3_length(direction), 1.0f,
				     RADIAN_TOLERANCE);
	}
}

// ----------------------------------------------------------- moving

// A DIAGONAL IS NOT FASTER THAN A STRAIGHT LINE. One key held and two keys held
// must cover the same ground in the same number of frames; the whole claim is
// that the direction is normalized before the speed is applied.
//
// Every pair is checked and not only forward-and-right, because normalizing the
// wrong two of the three components is a mistake that passes one pair.
static void a_diagonal_is_not_faster(void)
{
	static const struct {
		float forward;
		float right;
		float up;
	} PRESSES[] = {
		{ 1.0f, 0.0f, 0.0f },  { 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },  { 1.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f },  { 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },  { -1.0f, -1.0f, -1.0f },
	};
	// Enough frames that a root-three error is metres and not millimetres.
	const int FRAMES = 60;
	float straight = 0.0f;

	for (unsigned press = 0; press < sizeof(PRESSES) / sizeof(PRESSES[0]);
	     press++) {
		struct voe_render_camera camera = { 0 };
		voe_render_camera_input input = flying_still();
		voe_math_float3 start;
		float travelled;

		voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);
		start = camera.eye;

		input.forward = PRESSES[press].forward;
		input.right = PRESSES[press].right;
		input.up = PRESSES[press].up;
		for (int frame = 0; frame < FRAMES; frame++)
			voe_render_cube_camera_step(&camera, input, 0.0f, DT);

		travelled = voe_math_float3_length(
			voe_math_float3_sub(camera.eye, start));

		// The first press is one key, and every later one is measured
		// against it rather than against a constant this file would
		// have to keep in step with cube.c's speed.
		if (press == 0) {
			straight = travelled;
			VOE_TEST_CHECK(straight > 0.0f);
		} else {
			VOE_TEST_CHECK_FLOAT(travelled, straight,
					     METRE_TOLERANCE);
		}
	}
}

// OPPOSITE KEYS CANCEL AND THE CAMERA DOES NOT MOVE. It is the call site that
// adds and subtracts into these fields, so what reaches here is a nought — and a
// nought must not be normalized, because normalizing a vector of no length is a
// division by zero and standing still is the commonest thing a camera does.
static void standing_still_stays_still(void)
{
	struct voe_render_camera camera = { 0 };
	voe_math_float3 start;

	voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);
	start = camera.eye;

	for (int frame = 0; frame < 60; frame++)
		voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);

	VOE_TEST_CHECK_FLOAT(camera.eye.x, start.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(camera.eye.y, start.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(camera.eye.z, start.z, 0.0f);
}

// STRAFING STAYS HORIZONTAL WHILE LOOKING AT THE FLOOR, and up goes up.
//
// Both are the same decision read from two sides: `right` is the camera's and
// horizontal, `up` is the world's. A camera pitched hard down that strafed in
// its own frame would sink; one that rose along its own up would go forwards
// instead.
static void looking_down_does_not_tilt_the_controls(void)
{
	struct voe_render_camera camera = { 0 };
	voe_render_camera_input strafe = flying_still();
	voe_render_camera_input rise = flying_still();
	voe_math_float3 start;
	voe_math_float3 moved;

	voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);

	// Looking as far down as the clamp allows, along -Z to begin with.
	camera.yaw = 0.0f;
	camera.pitch = -1.5f;
	start = camera.eye;

	strafe.right = 1.0f;
	for (int frame = 0; frame < 60; frame++)
		voe_render_cube_camera_step(&camera, strafe, 0.0f, DT);

	moved = voe_math_float3_sub(camera.eye, start);
	// Right, at a yaw of nought, is +X and nothing else. A height that
	// changed is the bug.
	VOE_TEST_CHECK(moved.x > 0.0f);
	VOE_TEST_CHECK_FLOAT(moved.y, 0.0f, METRE_TOLERANCE);
	VOE_TEST_CHECK_FLOAT(moved.z, 0.0f, METRE_TOLERANCE);

	camera.eye = start;
	rise.up = 1.0f;
	for (int frame = 0; frame < 60; frame++)
		voe_render_cube_camera_step(&camera, rise, 0.0f, DT);

	moved = voe_math_float3_sub(camera.eye, start);
	// Straight up, from a camera looking almost straight down.
	VOE_TEST_CHECK(moved.y > 0.0f);
	VOE_TEST_CHECK_FLOAT(moved.x, 0.0f, METRE_TOLERANCE);
	VOE_TEST_CHECK_FLOAT(moved.z, 0.0f, METRE_TOLERANCE);
}

// `fast` MULTIPLIES AND DOES NOT REPLACE. Held, the camera covers more ground in
// the same frames; the claim is only that it is further, because how much
// further is a constant in cube.c and a test that named it would be a copy of it.
static void fast_is_faster(void)
{
	struct voe_render_camera camera = { 0 };
	struct voe_render_camera hurried = { 0 };
	voe_render_camera_input walk = flying_still();
	voe_render_camera_input run = flying_still();
	voe_math_float3 start;

	voe_render_cube_camera_step(&camera, flying_still(), 0.0f, DT);
	voe_render_cube_camera_step(&hurried, flying_still(), 0.0f, DT);
	start = camera.eye;

	walk.forward = 1.0f;
	run.forward = 1.0f;
	run.fast = true;
	for (int frame = 0; frame < 60; frame++) {
		voe_render_cube_camera_step(&camera, walk, 0.0f, DT);
		voe_render_cube_camera_step(&hurried, run, 0.0f, DT);
	}

	VOE_TEST_CHECK(voe_math_float3_length(
			       voe_math_float3_sub(hurried.eye, start)) >
		       voe_math_float3_length(
			       voe_math_float3_sub(camera.eye, start)));
}

int main(void)
{
	handover_does_not_jump();
	handing_back_keeps_nothing();
	mouse_turns_the_right_way();
	pitch_is_clamped();
	a_diagonal_is_not_faster();
	standing_still_stays_still();
	looking_down_does_not_tilt_the_controls();
	fast_is_faster();
	return voe_test_result();
}
