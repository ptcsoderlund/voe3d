// The projection matrix, and the render view built from a pose and a lens.
// Reverse-Z, 0..1, no Y negation — see the header for why
// each of those is not a preference.
//
// THE ARITHMETIC, WRITTEN OUT, BECAUSE THE SIGNS ARE THE WHOLE THING. Vectors
// are columns and the matrix is row-major, so for a point at (x, y, z, 1) in
// camera space — where the camera looks along -Z, so anything visible has a
// negative z:
//
//     clip.x = focal / aspect * x
//     clip.y = focal * y
//     clip.z = a * z + b
//     clip.w = -z
//
// and the depth a fragment gets is clip.z / clip.w. Asking for the near plane to
// come out at 1 and the far plane at 0 gives two equations:
//
//     at z = -near:  (-a * near + b) /  near = 1
//     at z = -far:   (-a * far  + b) /  far  = 0
//
// whose answer is a = near / (far - near) and b = near * far / (far - near).
// Nothing about that is standard and nothing about it is arbitrary; a sign
// dropped anywhere in it draws a picture that looks nearly right and clips in
// the wrong place.
//
// AS far GOES TO INFINITY THIS BECOMES a = 0, b = near, WHICH IS WHAT CARD 015
// HAD. That matrix ignored the far plane entirely because nothing had one to
// ignore; a camera carries one now, so the finite form is the honest one and the
// infinite one falls out of it rather than having to be chosen.
#include <3d/projection.h>
#include <base/assert.h>

#include <math.h>
#include <stddef.h>

voe_math_float4x4 voe_3d_projection(voe_scene_camera lens, float aspect)
{
	// The distance from the eye to a screen one unit high — the vertical
	// half-angle's cotangent. Everything else in here is that number
	// divided by something.
	float focal;
	float span;
	voe_math_float4x4 projection = { 0 };

	VOE_BASE_ASSERT(aspect > 0.0f, "a projection for a target with no area");
	VOE_BASE_ASSERT(lens.fov_y > 0.0f && lens.fov_y < 3.1415927f,
			"a projection for a camera whose field of view is not an angle");
	VOE_BASE_ASSERT(lens.near_plane > 0.0f,
			"a projection whose near plane is at or behind the eye");
	VOE_BASE_ASSERT(lens.far_plane > lens.near_plane,
			"a projection whose far plane is not further away than its near one");

	focal = 1.0f / tanf(lens.fov_y * 0.5f);
	span = lens.far_plane - lens.near_plane;

	// Divided by the aspect ratio and not multiplied. Multiplying instead
	// squashes a wide window rather than showing more of the scene, which is
	// the mistake that only shows up on a window nobody resized.
	projection.m[0][0] = focal / aspect;
	projection.m[1][1] = focal;
	projection.m[2][2] = lens.near_plane / span;
	projection.m[2][3] = lens.near_plane * lens.far_plane / span;

	// The perspective divide: w comes back as -z, which is positive for
	// anything the camera can see.
	projection.m[3][2] = -1.0f;
	return projection;
}

// The same two equations as the perspective one with w fixed at one:
// -a * near + b = 1 and -a * far + b = 0, so a = 1 / (far - near) and
// b = far / (far - near).
voe_math_float4x4 voe_3d_projection_orthographic(float half_width,
						 float half_height,
						 float near_plane,
						 float far_plane)
{
	voe_math_float4x4 projection = { 0 };
	float span = far_plane - near_plane;

	VOE_BASE_ASSERT(half_width > 0.0f && half_height > 0.0f,
			"an orthographic box with no area");
	VOE_BASE_ASSERT(span > 0.0f,
			"an orthographic box whose far plane is not past its near one");

	projection.m[0][0] = 1.0f / half_width;
	projection.m[1][1] = 1.0f / half_height;
	projection.m[2][2] = 1.0f / span;
	projection.m[2][3] = far_plane / span;
	projection.m[3][3] = 1.0f;
	VOE_BASE_ASSERT(projection.m[2][2] > 0.0f, "a box whose depth runs forwards");
	return projection;
}

bool voe_3d_view(voe_scene_transform pose, voe_scene_camera lens, float aspect,
		 voe_render_view *out)
{
	voe_math_float4x4 view;

	VOE_BASE_ASSERT(out != NULL, "a view with nowhere to go");
	if (!voe_scene_camera_view(pose, &view))
		return false;

	*out = (voe_render_view){ .view = view,
				  .projection = voe_3d_projection(lens, aspect),
				  .eye = { 0.0f, 0.0f, 0.0f },
				  .reserved = 0.0f };
	VOE_BASE_ASSERT(out->reserved == 0.0f, "a view with padding written");
	return true;
}
