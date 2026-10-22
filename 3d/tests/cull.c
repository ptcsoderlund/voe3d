// The view's planes against spheres: a camera's six, a cascade's box with its
// sun side open and shut, and a sphere's radius under an uneven scale.
//
// A SPHERE REACHING IN IS SEEN. The left-edge case puts a centre just outside
// the frustum and checks a radius that reaches across the plane keeps it, and
// one that does not loses it: the test is by distance in metres, not by centre.
// Needs no graphics card.
#include "../src/cull.h"

#include <3d/projection.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#define TOLERANCE 1e-5f

// A camera at the origin looking down -Z, 60° tall, 0.1 m to 100 m, square.
static struct voe_3d_cull a_camera_cull(void)
{
	voe_scene_transform pose = { .position = { 0.0f, 0.0f, 0.0f },
				     .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				     .scale = { 1.0f, 1.0f, 1.0f } };
	voe_scene_camera lens = { .fov_y = 1.0471976f,
				  .near_plane = 0.1f,
				  .far_plane = 100.0f };
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(pose, lens, 1.0f, &view));
	return voe_3d_cull_of(view, false);
}

static void a_camera_sees_ahead_not_behind_or_past_far(void)
{
	struct voe_3d_cull cull = a_camera_cull();

	VOE_TEST_CHECK(cull.count == 6);
	VOE_TEST_CHECK(voe_3d_cull_sees(&cull, (voe_math_float3){ 0, 0, -10 }, 1));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&cull, (voe_math_float3){ 0, 0, 10 }, 1));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&cull, (voe_math_float3){ 0, 0, -110 }, 1));
}

// 10 m ahead the left edge is at x = -10 tan 30° ≈ -5.77; a centre at -6 is
// about 0.2 m outside it, measured along the plane's normal.
static void a_sphere_reaching_in_at_the_left_is_seen(void)
{
	struct voe_3d_cull cull = a_camera_cull();
	voe_math_float3 centre = { -6.0f, 0.0f, -10.0f };

	VOE_TEST_CHECK(voe_3d_cull_sees(&cull, centre, 1.0f));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&cull, centre, 0.1f));
}

// A cascade's lens: a 4 × 4 m box from 1 m to 11 m, seen down -Z from the
// origin, the sun behind the eye along +Z.
static struct voe_3d_cull a_cascade_cull(bool sun_side_open)
{
	voe_render_view view = {
		.view = voe_math_float4x4_identity(),
		.projection = voe_3d_projection_orthographic(4.0f, 4.0f, 1.0f,
							     11.0f),
	};

	return voe_3d_cull_of(view, sun_side_open);
}

static void a_cascade_keeps_casters_toward_the_sun_when_open(void)
{
	struct voe_3d_cull open = a_cascade_cull(true);
	struct voe_3d_cull shut = a_cascade_cull(false);
	voe_math_float3 sunward = { 0.0f, 0.0f, 50.0f };
	voe_math_float3 aside = { 10.0f, 0.0f, -5.0f };

	VOE_TEST_CHECK(open.count == 5);
	VOE_TEST_CHECK(shut.count == 6);
	VOE_TEST_CHECK(voe_3d_cull_sees(&open, sunward, 1.0f));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&shut, sunward, 1.0f));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&open, aside, 1.0f));
	VOE_TEST_CHECK(!voe_3d_cull_sees(&shut, aside, 1.0f));
	VOE_TEST_CHECK(voe_3d_cull_sees(&shut, (voe_math_float3){ 0, 0, -5 }, 1));
}

static void a_scaled_radius_is_its_longest_axis(void)
{
	voe_math_float4x4 world =
		voe_math_float4x4_from_scale((voe_math_float3){ 1.0f, 3.0f, 2.0f });

	VOE_TEST_CHECK_FLOAT(voe_3d_cull_radius(world, 0.5f), 1.5f, TOLERANCE);
}

int main(void)
{
	a_camera_sees_ahead_not_behind_or_past_far();
	a_sphere_reaching_in_at_the_left_is_seen();
	a_cascade_keeps_casters_toward_the_sun_when_open();
	a_scaled_radius_is_its_longest_axis();

	return voe_test_result();
}
