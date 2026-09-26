// The projection matrix: that depth really runs backwards, that the aspect ratio
// divides rather than multiplies, and that there is no Y flip in it; and the
// render view a pose and a lens become.
//
// ALL THREE OF THESE DRAW A PLAUSIBLE PICTURE WHEN THEY ARE WRONG, WHICH IS WHY
// THEY ARE CHECKED ON THE CPU. Reversed depth the right way round and reversed
// depth the wrong way round both fill the screen; an aspect ratio multiplied
// instead of divided looks right on a square window; a negated Y row looks right
// until something is culled. None of them needs a graphics card to disagree
// with, so none of them waits for one.
//
// THIS WAS render/tests/matrix.c's FIRST HALF UNTIL CARD 018. The projection
// moved to `3d` when the camera moved to `scene`, and the checks came with it.
#include <3d/projection.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <scene/camera_component.h>

#include <testing/test.h>

#define TOLERANCE 1e-5f

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static voe_scene_camera a_camera(void)
{
	voe_scene_camera camera = {
		.fov_y = 1.0471976f,
		.near_plane = NEAR_PLANE,
		.far_plane = FAR_PLANE,
	};

	return camera;
}

// A point in camera space through the projection and the divide, which is what a
// fragment's depth is.
static float depth_of(voe_math_float4x4 projection, float z)
{
	voe_math_float4 point = { 0.0f, 0.0f, z, 1.0f };
	voe_math_float4 clip = voe_math_float4x4_mul_float4(projection, point);

	VOE_TEST_CHECK(clip.w != 0.0f);
	if (clip.w == 0.0f)
		return 0.0f;
	return clip.z / clip.w;
}

// Near at one and far at nought, which is the opposite of every tutorial and is
// this engine's convention (CLAUDE.md). The depth buffer is cleared to 0 and the
// comparison is GREATER, so getting this backwards rejects every fragment.
static void depth_runs_backwards(void)
{
	voe_math_float4x4 projection = voe_3d_projection(a_camera(), 1.0f);
	float near_depth = depth_of(projection, -NEAR_PLANE);
	float far_depth = depth_of(projection, -FAR_PLANE);
	float middle = depth_of(projection, -1.0f);

	VOE_TEST_CHECK_FLOAT(near_depth, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(far_depth, 0.0f, TOLERANCE);

	// And it is monotonic in between, so nothing sorts the wrong way round
	// halfway out. A point one metre away is nearer than the far plane and
	// further than the near one, so its depth is between the two.
	VOE_TEST_CHECK(middle < near_depth);
	VOE_TEST_CHECK(middle > far_depth);

	// Most of the range is spent near the camera, which is the whole reason
	// for reversing it: a float depth buffer has its precision near zero
	// and zero is now the far plane.
	VOE_TEST_CHECK(middle < 0.5f);
}

// A wider target shows more of the scene; it does not stretch it. An aspect
// ratio multiplied instead of divided fails here and nowhere else.
static void the_aspect_ratio_divides(void)
{
	voe_math_float4x4 square = voe_3d_projection(a_camera(), 1.0f);
	voe_math_float4x4 wide = voe_3d_projection(a_camera(), 2.0f);
	voe_math_float4 point = { 1.0f, 1.0f, -2.0f, 1.0f };
	voe_math_float4 in_square = voe_math_float4x4_mul_float4(square, point);
	voe_math_float4 in_wide = voe_math_float4x4_mul_float4(wide, point);

	// Twice as wide, so the same point is half as far across the frame.
	VOE_TEST_CHECK_FLOAT(in_wide.x / in_wide.w,
			     in_square.x / in_square.w * 0.5f, TOLERANCE);

	// And exactly as far up it: the vertical field of view is the camera's
	// and does not follow the window.
	VOE_TEST_CHECK_FLOAT(in_wide.y / in_wide.w,
			     in_square.y / in_square.w, TOLERANCE);
}

// Nothing in the projection flips Y — the one flip in the engine is the
// viewport's negative height, inside `render`. Flipping here as well is
// invisible until something is culled.
static void nothing_here_flips_y(void)
{
	voe_math_float4x4 projection = voe_3d_projection(a_camera(), 1.0f);
	voe_math_float4 up = { 0.0f, 1.0f, -2.0f, 1.0f };
	voe_math_float4 clip = voe_math_float4x4_mul_float4(projection, up);

	VOE_TEST_CHECK(projection.m[1][1] > 0.0f);
	VOE_TEST_CHECK(clip.y / clip.w > 0.0f);

	// And nothing else in the matrix has anything in it: a projection with a
	// translation or a shear in it is a projection somebody has been
	// correcting something with.
	VOE_TEST_CHECK_FLOAT(projection.m[0][1], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[0][2], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[0][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[1][0], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[1][2], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[1][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[3][0], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[3][1], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[3][3], 0.0f, 0.0f);

	// The perspective divide is the only thing in the last row.
	VOE_TEST_CHECK_FLOAT(projection.m[3][2], -1.0f, 0.0f);
}

// The centre of the frame is the centre of the frame, whatever the field of view
// and whatever the target's shape.
static void the_centre_stays_centred(void)
{
	voe_scene_camera camera = a_camera();
	voe_math_float4x4 projection;
	voe_math_float4 ahead = { 0.0f, 0.0f, -5.0f, 1.0f };
	voe_math_float4 clip;

	camera.fov_y = 0.5f;
	projection = voe_3d_projection(camera, 3.0f);
	clip = voe_math_float4x4_mul_float4(projection, ahead);

	VOE_TEST_CHECK_FLOAT(clip.x, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(clip.y, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(clip.w, 5.0f, TOLERANCE);
}

// A pose at (0, 0, 5) sees from there: the view is scene's own, about the
// pose's position, so the eye is nought (ADR-0250), and the projection is the
// lens's.
static void a_view_is_a_pose_and_a_lens(void)
{
	voe_scene_transform pose = { .position = { 0.0f, 0.0f, 5.0f },
				     .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				     .scale = { 1.0f, 1.0f, 1.0f } };
	voe_math_float4x4 expected_view;
	voe_math_float4x4 expected_projection = voe_3d_projection(a_camera(), 2.0f);
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_scene_camera_view(pose, &expected_view));
	VOE_TEST_CHECK(voe_3d_view(pose, a_camera(), 2.0f, &view));
	VOE_TEST_CHECK_FLOAT(view.eye.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(view.eye.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(view.eye.z, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(view.reserved, 0.0f, 0.0f);
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			VOE_TEST_CHECK_FLOAT(view.view.m[row][column],
					     expected_view.m[row][column], 0.0f);
			VOE_TEST_CHECK_FLOAT(view.projection.m[row][column],
					     expected_projection.m[row][column],
					     0.0f);
		}
	}
}

// A pose scaled to nothing on an axis has no inverse and sees nothing; the
// view it was handed is not touched.
static void a_flattened_pose_sees_nothing(void)
{
	voe_scene_transform pose = { .position = { 0.0f, 0.0f, 5.0f },
				     .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				     .scale = { 1.0f, 0.0f, 1.0f } };
	voe_render_view view = { .eye = { 7.0f, 7.0f, 7.0f } };

	VOE_TEST_CHECK(!voe_3d_view(pose, a_camera(), 1.0f, &view));
	VOE_TEST_CHECK_FLOAT(view.eye.x, 7.0f, 0.0f);
}

// The sun's box runs backwards too: its near plane at depth one, its far at
// nought, and a corner of it at ±1 across and up, w untouched.
static void the_box_runs_backwards(void)
{
	voe_math_float4x4 box =
		voe_3d_projection_orthographic(4.0f, 2.0f, 1.0f, 11.0f);
	voe_math_float4 near_corner = { 4.0f, -2.0f, -1.0f, 1.0f };
	voe_math_float4 far_corner = { -4.0f, 2.0f, -11.0f, 1.0f };
	voe_math_float4 near_clip = voe_math_float4x4_mul_float4(box, near_corner);
	voe_math_float4 far_clip = voe_math_float4x4_mul_float4(box, far_corner);

	VOE_TEST_CHECK_FLOAT(near_clip.z, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(far_clip.z, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(near_clip.x, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(near_clip.y, -1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(far_clip.x, -1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(far_clip.y, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(near_clip.w, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(far_clip.w, 1.0f, 0.0f);
}

int main(void)
{
	depth_runs_backwards();
	the_box_runs_backwards();
	the_aspect_ratio_divides();
	nothing_here_flips_y();
	the_centre_stays_centred();
	a_view_is_a_pose_and_a_lens();
	a_flattened_pose_sees_nothing();

	return voe_test_result();
}
