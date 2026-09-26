// The sun's cascades fitted to a view: that the splits rise to the reach, that
// every slice lands inside its cascade's box, and that the fit is stable — a
// fixed world point moves by whole texels as the eye moves and turns, near the
// origin and 100 km out — and that a sun straight down still has a basis.
// Needs no graphics card.
#include <3d/projection.h>
#include <3d/shadow_cascades.h>
#include <math/double3.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#include <math.h>

#define FOV_Y 1.0471976f
#define ASPECT 1.7777778f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

static voe_math_float3 a_sun(void)
{
	return voe_math_float3_normalize((voe_math_float3){ 0.3f, -1.0f, 0.2f });
}

// A camera at `at`, turned `turn` radians about Y, and its fit.
static voe_3d_shadow_cascades fit_at(voe_math_double3 at, float turn,
				     voe_math_float3 sun, voe_render_view *view)
{
	voe_scene_camera lens = { .fov_y = FOV_Y, .near_plane = NEAR_PLANE,
				  .far_plane = FAR_PLANE };
	voe_scene_transform pose = {
		.position = at,
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, turn),
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_3d_view(pose, lens, ASPECT, view));
	return voe_3d_shadow_cascades_fit(*view, at, sun, VOE_3D_SHADOW_TEXELS);
}

static voe_math_float4 clip_of(voe_math_float4x4 cascade, voe_math_float3 point)
{
	return voe_math_float4x4_mul_float4(
		cascade, (voe_math_float4){ point.x, point.y, point.z, 1.0f });
}

static void the_splits_rise_to_the_reach(void)
{
	voe_render_view view;
	voe_3d_shadow_cascades fit =
		fit_at((voe_math_double3){ 0.0, 0.0, 0.0 }, 0.0f, a_sun(), &view);

	VOE_TEST_CHECK_INT(fit.shadow.count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK(fit.shadow.splits[0] > NEAR_PLANE);
	for (int i = 1; i < VOE_RENDER_SHADOW_CASCADES; i++)
		VOE_TEST_CHECK(fit.shadow.splits[i] > fit.shadow.splits[i - 1]);
	VOE_TEST_CHECK_FLOAT(fit.shadow.splits[VOE_RENDER_SHADOW_CASCADES - 1],
			     fminf(FAR_PLANE, VOE_3D_SHADOW_REACH), 1e-3f);
}

// Each slice's eight corners, built straight from the lens and turned by the
// view, fall inside the cascade's clip box.
static void every_slice_is_inside_its_box(voe_math_double3 at, float turn)
{
	voe_render_view view;
	voe_3d_shadow_cascades fit = fit_at(at, turn, a_sun(), &view);
	voe_math_float4x4 unview = voe_math_float4x4_inverse(view.view);
	float tangent = tanf(FOV_Y * 0.5f);
	float from = NEAR_PLANE;

	for (int i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		for (int corner = 0; corner < 8; corner++) {
			float d = (corner & 4) ? fit.shadow.splits[i] : from;
			voe_math_float3 local = {
				((corner & 1) ? 1.0f : -1.0f) * d * tangent * ASPECT,
				((corner & 2) ? 1.0f : -1.0f) * d * tangent, -d
			};
			voe_math_float4 clip = clip_of(
				fit.shadow.cascades[i],
				voe_math_float4x4_transform_point(unview, local));

			VOE_TEST_CHECK(fabsf(clip.x) <= 1.0f);
			VOE_TEST_CHECK(fabsf(clip.y) <= 1.0f);
			VOE_TEST_CHECK(clip.z >= 0.0f && clip.z <= 1.0f);
			VOE_TEST_CHECK_FLOAT(clip.w, 1.0f, 1e-6f);
		}
		from = fit.shadow.splits[i];
	}
}

static float whole_distance(float value)
{
	return fabsf(value - roundf(value));
}

// The eye moved a few centimetres and turned 20°: a fixed world point's texel
// in every cascade moves by a whole number, and no texel changes size.
static void a_moved_eye_moves_the_map_by_whole_texels(voe_math_double3 at)
{
	voe_math_double3 moved = voe_math_double3_add(
		at, (voe_math_double3){ 0.37, 0.11, 0.21 });
	voe_math_double3 point = voe_math_double3_add(
		at, (voe_math_double3){ 3.0, 0.5, -8.0 });
	voe_render_view view;
	voe_3d_shadow_cascades before = fit_at(at, 0.0f, a_sun(), &view);
	voe_3d_shadow_cascades after = fit_at(moved, 0.34906585f, a_sun(), &view);
	voe_math_float3 from_before =
		voe_math_double3_to_float3(voe_math_double3_sub(point, at));
	voe_math_float3 from_after =
		voe_math_double3_to_float3(voe_math_double3_sub(point, moved));
	float half = 0.5f * (float)VOE_3D_SHADOW_TEXELS;

	for (int i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		voe_math_float4 a = clip_of(before.shadow.cascades[i], from_before);
		voe_math_float4 b = clip_of(after.shadow.cascades[i], from_after);

		VOE_TEST_CHECK_FLOAT(after.shadow.texels[i],
				     before.shadow.texels[i], 0.0f);
		VOE_TEST_CHECK(whole_distance((b.x - a.x) * half) < 1e-3f);
		VOE_TEST_CHECK(whole_distance((b.y - a.y) * half) < 1e-3f);
	}
}

static void a_sun_straight_down_has_a_basis(void)
{
	voe_render_view view;
	voe_3d_shadow_cascades fit =
		fit_at((voe_math_double3){ 0.0, 0.0, 0.0 }, 0.0f,
		       (voe_math_float3){ 0.0f, -1.0f, 0.0f }, &view);

	for (int i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++)
		for (int row = 0; row < 4; row++)
			for (int column = 0; column < 4; column++)
				VOE_TEST_CHECK(isfinite(
					fit.shadow.cascades[i].m[row][column]));
}

int main(void)
{
	voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_math_double3 far_out = { 100000.0, 0.0, 0.0 };

	the_splits_rise_to_the_reach();
	every_slice_is_inside_its_box(origin, 0.0f);
	every_slice_is_inside_its_box(origin, 0.34906585f);
	a_moved_eye_moves_the_map_by_whole_texels(origin);
	every_slice_is_inside_its_box(far_out, 0.0f);
	every_slice_is_inside_its_box(far_out, 0.34906585f);
	a_moved_eye_moves_the_map_by_whole_texels(far_out);
	a_sun_straight_down_has_a_basis();

	return voe_test_result();
}
