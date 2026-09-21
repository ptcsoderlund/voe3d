// The move gizmo's arithmetic: how big it is, which handle a ray meets and
// where on that handle a drag starts.
//
// NONE OF IT NEEDS A GRAPHICS CARD, AND THAT IS THE POINT OF THE MODULE. A
// gizmo is one position, one camera and six handles of plain geometry
// (3d/gizmo.h), so every claim here is a ray in and a number out on a build box
// with no Vulkan. What a person sees of it — the triangles and the pass that
// draws them — is checked where those are written.
//
// The gizmo below stands away from the world's origin on purpose: an axis test
// that passes for a gizmo at (0, 0, 0) can be one that forgot the origin
// entirely.
#include <3d/gizmo.h>
#include <3d/pick.h>
#include <3d/projection.h>

#include <math/float3.h>

#include <platform/window.h>

#include <render/device.h>

#include <scene/camera_component.h>

#include <testing/test.h>

#define WIDTH 640
#define HEIGHT 480

// A shaft of one metre, so every fraction in the header reads as itself here.
#define SHAFT 1.0f

// Where the gizmo stands, and far enough from everything that a ray aimed five
// metres away starts outside it.
#define REACH 5.0f

static const voe_math_float3 WHERE = { 1.0f, 2.0f, 3.0f };

// The world's three axes, and the direction each arrow is looked at from: any
// unit vector across that arrow, so the ray meets it side on rather than down
// its own line.
static const voe_math_float3 AXIS[3] = { { 1.0f, 0.0f, 0.0f },
					 { 0.0f, 1.0f, 0.0f },
					 { 0.0f, 0.0f, 1.0f } };
static const voe_math_float3 ACROSS[3] = { { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 0.0f, 0.0f } };

static voe_3d_gizmo a_gizmo(void)
{
	return (voe_3d_gizmo){
		.origin = WHERE,
		.eye = { 1.0f, 2.0f, 13.0f },
		.shaft = SHAFT,
	};
}

// A ray at `target` from REACH metres away along `from`, which is a unit vector.
static voe_3d_ray aimed_at(voe_math_float3 target, voe_math_float3 from)
{
	return (voe_3d_ray){
		.origin = voe_math_float3_add(
			target, voe_math_float3_scale(from, REACH)),
		.direction = voe_math_float3_neg(from),
	};
}

static voe_math_float3 along(int axis, float metres)
{
	return voe_math_float3_scale(AXIS[axis], metres);
}

// A camera `distance` metres in front of the world's origin, looking at it, and
// the two matrices a pass would be opened with.
static voe_render_view the_view(float distance)
{
	voe_scene_camera camera = {
		.eye = { 0.0f, 0.0f, distance },
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	return (voe_render_view){
		.view = voe_scene_camera_view(camera),
		.projection = voe_3d_projection(camera,
						(float)WIDTH / (float)HEIGHT),
		.eye = camera.eye,
	};
}

// A ray across the middle of each arrow meets that arrow and not its two
// neighbours, which share the arrow's near end.
static void each_arrow_is_hit_across_its_middle(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	int axis;

	for (axis = 0; axis < 3; axis++) {
		voe_math_float3 middle = voe_math_float3_add(
			gizmo.origin, along(axis, 0.5f * SHAFT));

		VOE_TEST_CHECK_INT(voe_3d_gizmo_hit(
					   gizmo, aimed_at(middle,
							   ACROSS[axis])),
				   VOE_3D_GIZMO_X + axis);
	}
}

// A ray through the middle of each plane square meets that plane: the square
// lies between the near corner and the far one on both of its own axes, so its
// middle is halfway between them on each.
static void each_square_is_hit_through_its_middle(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	float middle = SHAFT * (VOE_3D_GIZMO_PLANE_NEAR +
				VOE_3D_GIZMO_PLANE_SIDE * 0.5f);
	int plane;

	for (plane = 0; plane < 3; plane++) {
		voe_math_float3 centre = voe_math_float3_add(
			gizmo.origin,
			voe_math_float3_add(along(plane, middle),
					    along((plane + 1) % 3, middle)));

		VOE_TEST_CHECK_INT(
			voe_3d_gizmo_hit(gizmo,
					 aimed_at(centre,
						  AXIS[(plane + 2) % 3])),
			VOE_3D_GIZMO_XY + plane);
	}
}

// A ray into the space beside the gizmo meets none of the six, and so does
// every ray at a gizmo of no size.
static void empty_space_hits_nothing(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	voe_math_float3 beside = voe_math_float3_add(
		gizmo.origin, (voe_math_float3){ 10.0f, 10.0f, 0.0f });
	voe_3d_ray ray = aimed_at(beside, AXIS[2]);

	VOE_TEST_CHECK_INT(voe_3d_gizmo_hit(gizmo, ray), VOE_3D_GIZMO_NONE);

	gizmo.shaft = 0.0f;
	VOE_TEST_CHECK_INT(voe_3d_gizmo_hit(gizmo,
					    aimed_at(gizmo.origin, AXIS[2])),
			   VOE_3D_GIZMO_NONE);
}

// The same camera twice as far away makes the gizmo twice as big in metres,
// which is what covering the same pixels at any distance means.
static void twice_as_far_is_twice_the_shaft(void)
{
	voe_math_float3 origin = { 0.0f, 0.0f, 0.0f };
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_gizmo near_gizmo =
		voe_3d_gizmo_at(origin, the_view(5.0f), size, 90.0f);
	voe_3d_gizmo far_gizmo =
		voe_3d_gizmo_at(origin, the_view(10.0f), size, 90.0f);

	VOE_TEST_CHECK(near_gizmo.shaft > 0.0f);
	VOE_TEST_CHECK_FLOAT(far_gizmo.shaft, near_gizmo.shaft * 2.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_gizmo.eye.z, 10.0f, 1e-6f);
}

// A grab on an axis moves along that axis alone: the two coordinates that are
// not the axis's are the origin's own, which is what makes the drag one
// direction.
static void an_axis_grab_keeps_the_other_two(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	voe_math_float3 on_x = voe_math_float3_add(gizmo.origin,
						   along(0, 0.5f * SHAFT));
	voe_math_float3 grabbed = { 0.0f, 0.0f, 0.0f };

	VOE_TEST_CHECK(voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_X,
					 aimed_at(on_x, AXIS[2]), &grabbed));
	VOE_TEST_CHECK_FLOAT(grabbed.x, gizmo.origin.x + 0.5f * SHAFT, 1e-5f);
	VOE_TEST_CHECK_FLOAT(grabbed.y, gizmo.origin.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT(grabbed.z, gizmo.origin.z, 1e-5f);
}

// A grab on a plane square lands in that plane, so the coordinate the plane is
// normal to is the origin's.
static void a_plane_grab_keeps_its_normal(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	voe_math_float3 in_zx = voe_math_float3_add(
		gizmo.origin,
		voe_math_float3_add(along(2, 0.4f * SHAFT),
				    along(0, 0.4f * SHAFT)));
	voe_math_float3 grabbed = { 0.0f, 0.0f, 0.0f };

	VOE_TEST_CHECK(voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_ZX,
					 aimed_at(in_zx, AXIS[1]), &grabbed));
	VOE_TEST_CHECK_FLOAT(grabbed.y, gizmo.origin.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT(grabbed.x, gizmo.origin.x + 0.4f * SHAFT, 1e-5f);
	VOE_TEST_CHECK_FLOAT(grabbed.z, gizmo.origin.z + 0.4f * SHAFT, 1e-5f);
}

// A ray lying in a square's own plane names no point of it, and neither does no
// handle at all: both say false and leave the answer as it was.
static void a_ray_in_a_plane_is_refused(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	voe_3d_ray across = { .origin = voe_math_float3_add(gizmo.origin,
							    along(1, REACH)),
			      .direction = AXIS[0] };
	voe_math_float3 untouched = { 9.0f, 9.0f, 9.0f };

	VOE_TEST_CHECK(!voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_ZX, across,
					  &untouched));
	VOE_TEST_CHECK(!voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_NONE,
					  aimed_at(gizmo.origin, AXIS[2]),
					  &untouched));
	VOE_TEST_CHECK_FLOAT(untouched.x, 9.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(untouched.y, 9.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(untouched.z, 9.0f, 0.0f);
}

int main(void)
{
	each_arrow_is_hit_across_its_middle();
	each_square_is_hit_through_its_middle();
	empty_space_hits_nothing();
	twice_as_far_is_twice_the_shaft();
	an_axis_grab_keeps_the_other_two();
	a_plane_grab_keeps_its_normal();
	a_ray_in_a_plane_is_refused();
	return voe_test_result();
}
