// The move gizmo's arithmetic: how big it is, which handle a ray meets, where on
// that handle a drag starts, and the triangles it is drawn as.
//
// NONE OF IT NEEDS A GRAPHICS CARD, AND THAT IS THE POINT OF THE MODULE. A
// gizmo is one position, one camera and six handles of plain geometry
// (3d/gizmo.h), so every claim here is a ray or an eye in and numbers out on a
// build box with no Vulkan — the triangles included, which are counted and
// measured here and drawn where the pass that draws them is written.
//
// The gizmo below stands away from the world's origin on purpose: an axis test
// that passes for a gizmo at (0, 0, 0) can be one that forgot the origin
// entirely.
#include <3d/gizmo.h>
#include <3d/pick.h>
#include <3d/projection.h>

#include <base/arena.h>

#include <math/double3.h>
#include <math/float3.h>

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

// Enough for the four arrays one build pushes, many times over.
#define SCRATCH (1024 * 1024)

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
		.origin = voe_math_double3_from_float3(WHERE),
		.eye = { 1.0, 2.0, 13.0 },
		.shaft = SHAFT,
	};
}

// A ray at `target` from REACH metres away along `from`, which is a unit vector.
static voe_3d_ray aimed_at(voe_math_float3 target, voe_math_float3 from)
{
	return (voe_3d_ray){
		.origin = voe_math_double3_from_float3(voe_math_float3_add(
			target, voe_math_float3_scale(from, REACH))),
		.direction = voe_math_float3_neg(from),
	};
}

static voe_math_float3 along(int axis, float metres)
{
	return voe_math_float3_scale(AXIS[axis], metres);
}

// A camera `distance` metres in front of the world's origin, looking at it, and
// the two matrices a pass would be opened with, through voe_3d_view.
static voe_render_view the_view(float distance)
{
	voe_scene_transform pose = {
		.position = { 0.0f, 0.0f, distance },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(pose, lens, (float)WIDTH / (float)HEIGHT,
				   &view));
	return view;
}

// A ray across the middle of each arrow meets that arrow and not its two
// neighbours, which share the arrow's near end.
static void each_arrow_is_hit_across_its_middle(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	int axis;

	for (axis = 0; axis < 3; axis++) {
		voe_math_float3 middle = voe_math_float3_add(
			WHERE, along(axis, 0.5f * SHAFT));

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
			WHERE,
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
		WHERE, (voe_math_float3){ 10.0f, 10.0f, 0.0f });
	voe_3d_ray ray = aimed_at(beside, AXIS[2]);

	VOE_TEST_CHECK_INT(voe_3d_gizmo_hit(gizmo, ray), VOE_3D_GIZMO_NONE);

	gizmo.shaft = 0.0f;
	VOE_TEST_CHECK_INT(voe_3d_gizmo_hit(gizmo,
					    aimed_at(WHERE, AXIS[2])),
			   VOE_3D_GIZMO_NONE);
}

// The same camera twice as far away makes the gizmo twice as big in metres,
// which is what covering the same pixels at any distance means.
static void twice_as_far_is_twice_the_shaft(void)
{
	voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_gizmo near_gizmo =
		voe_3d_gizmo_at(origin, the_view(5.0f),
				(voe_math_double3){ 0.0, 0.0, 5.0 }, size, 90.0f);
	voe_3d_gizmo far_gizmo =
		voe_3d_gizmo_at(origin, the_view(10.0f),
				(voe_math_double3){ 0.0, 0.0, 10.0 }, size, 90.0f);

	VOE_TEST_CHECK(near_gizmo.shaft > 0.0f);
	VOE_TEST_CHECK_FLOAT(far_gizmo.shaft, near_gizmo.shaft * 2.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_gizmo.eye.z, 10.0f, 1e-6f);
}

// A gizmo nobody can see has no size (084): behind the eye at the distance
// the editor's assert was flown to, and a centimetre in front of the eye but
// 68 m off to the side; and either builds nothing rather than a label of no direction.
static void an_unseen_gizmo_has_no_shaft(voe_base_arena *arena)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_math_double3 eye = { 0.0, 0.0, 5.0 };
	voe_3d_gizmo behind =
		voe_3d_gizmo_at((voe_math_double3){ 18.0, -61.0, 40.0 },
				the_view(5.0f), eye, size, 80.0f);
	voe_3d_gizmo beside =
		voe_3d_gizmo_at((voe_math_double3){ 68.0, 0.0, 4.99 },
				the_view(5.0f), eye, size, 80.0f);
	voe_3d_gizmo_mesh plain = { .vertex_count = 9 };
	voe_3d_gizmo_mesh under = { .vertex_count = 9 };

	VOE_TEST_CHECK(behind.shaft == 0.0f);
	VOE_TEST_CHECK(beside.shaft == 0.0f);
	VOE_TEST_CHECK(!voe_3d_gizmo_quads(behind, VOE_3D_GIZMO_NONE, arena,
					   &plain, &under));
	VOE_TEST_CHECK(!voe_3d_gizmo_quads(beside, VOE_3D_GIZMO_NONE, arena,
					   &plain, &under));
	VOE_TEST_CHECK_INT(plain.vertex_count, 9);
}

// A grab on an axis moves along that axis alone: the two coordinates that are
// not the axis's are the origin's own, which is what makes the drag one
// direction.
static void an_axis_grab_keeps_the_other_two(void)
{
	voe_3d_gizmo gizmo = a_gizmo();
	voe_math_float3 on_x = voe_math_float3_add(WHERE,
						   along(0, 0.5f * SHAFT));
	voe_math_double3 grabbed = { 0.0, 0.0, 0.0 };

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
		WHERE,
		voe_math_float3_add(along(2, 0.4f * SHAFT),
				    along(0, 0.4f * SHAFT)));
	voe_math_double3 grabbed = { 0.0, 0.0, 0.0 };

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
	voe_3d_ray across = { .origin = voe_math_double3_from_float3(
					      voe_math_float3_add(
						      WHERE, along(1, REACH))),
			      .direction = AXIS[0] };
	voe_math_double3 untouched = { 9.0, 9.0, 9.0 };

	VOE_TEST_CHECK(!voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_ZX, across,
					  &untouched));
	VOE_TEST_CHECK(!voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_NONE,
					  aimed_at(WHERE, AXIS[2]),
					  &untouched));
	VOE_TEST_CHECK_FLOAT(untouched.x, 9.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(untouched.y, 9.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(untouched.z, 9.0f, 0.0f);
}


// A gizmo of the usual size seen from `eye`.
static voe_3d_gizmo seen_from(voe_math_float3 eye)
{
	return (voe_3d_gizmo){ .origin = voe_math_double3_from_float3(WHERE),
			       .eye = voe_math_double3_from_float3(eye),
			       .shaft = SHAFT };
}

// What one handle costs: an arrow is a shaft quad and a head triangle, a square
// is one quad.
static uint32_t vertices_of(voe_3d_gizmo_handle handle)
{
	return handle <= VOE_3D_GIZMO_Z ? 4 + 3 : 4;
}

// The two meshes together are the two constants and never more, whichever handle
// is marked and when none is — which is what a program sizes its transient pools
// from.
static void the_two_meshes_fit_the_constants(voe_base_arena *arena)
{
	voe_3d_gizmo gizmo = seen_from((voe_math_float3){ 4.0f, 6.0f, 9.0f });
	int marked;

	for (marked = VOE_3D_GIZMO_NONE; marked <= VOE_3D_GIZMO_ZX; marked++) {
		voe_3d_gizmo_mesh plain = { 0 };
		voe_3d_gizmo_mesh under = { 0 };

		VOE_TEST_CHECK(voe_3d_gizmo_quads(gizmo,
						  (voe_3d_gizmo_handle)marked,
						  arena, &plain, &under));
		VOE_TEST_CHECK(plain.vertex_count + under.vertex_count <=
			       VOE_3D_GIZMO_VERTICES);
		VOE_TEST_CHECK(plain.index_count + under.index_count <=
			       VOE_3D_GIZMO_INDICES);
		VOE_TEST_CHECK(plain.vertex_count > 0);
	}
}

// Every triangle of both meshes faces the eye, because the pipeline culls back
// faces: the normal worked out from its own three corners points at the eye and
// not away from it. The eyes below stand on both sides of all three planes, so
// every square is checked from either side of itself.
static void every_triangle_faces_the_eye(voe_base_arena *arena)
{
	const voe_math_float3 EYES[2] = { { 5.0f, 7.0f, 11.0f },
					  { -4.0f, -8.0f, -6.0f } };
	int which;

	for (which = 0; which < 2; which++) {
		voe_3d_gizmo gizmo = seen_from(EYES[which]);
		voe_3d_gizmo_mesh mesh[2] = { { 0 }, { 0 } };
		int side;

		VOE_TEST_CHECK(voe_3d_gizmo_quads(gizmo, VOE_3D_GIZMO_Y, arena,
						  &mesh[0], &mesh[1]));
		for (side = 0; side < 2; side++)
			for (uint32_t i = 0; i + 2 < mesh[side].index_count;
			     i += 3) {
				const voe_render_vertex *v =
					mesh[side].vertices;
				const uint32_t *at = &mesh[side].indices[i];
				voe_math_float3 a = v[at[0]].position;
				voe_math_float3 normal = voe_math_float3_cross(
					voe_math_float3_sub(v[at[1]].position,
							    a),
					voe_math_float3_sub(v[at[2]].position,
							    a));

				// The quads are about the eye, so the eye
				// is at nought in their space.
				VOE_TEST_CHECK(voe_math_float3_dot(
						       normal,
						       voe_math_float3_neg(a)) >
					       0.0f);
			}
	}
}

// Marking a handle takes that handle's triangles out of the plain mesh and puts
// that many into the marked one — the same triangles at other widths, which is
// why nothing is gained or lost. Marking nothing leaves an empty mesh.
static void marking_moves_one_handle_across(voe_base_arena *arena)
{
	voe_3d_gizmo gizmo = seen_from((voe_math_float3){ 4.0f, 6.0f, 9.0f });
	voe_3d_gizmo_mesh at_rest = { 0 };
	voe_3d_gizmo_mesh empty = { 0 };
	int marked;

	VOE_TEST_CHECK(voe_3d_gizmo_quads(gizmo, VOE_3D_GIZMO_NONE, arena,
					  &at_rest, &empty));
	VOE_TEST_CHECK_INT(empty.vertex_count, 0);
	VOE_TEST_CHECK_INT(empty.index_count, 0);

	for (marked = VOE_3D_GIZMO_X; marked <= VOE_3D_GIZMO_ZX; marked++) {
		voe_3d_gizmo_mesh plain = { 0 };
		voe_3d_gizmo_mesh under = { 0 };

		VOE_TEST_CHECK(voe_3d_gizmo_quads(gizmo,
						  (voe_3d_gizmo_handle)marked,
						  arena, &plain, &under));
		VOE_TEST_CHECK_INT(under.vertex_count,
				   vertices_of((voe_3d_gizmo_handle)marked));
		VOE_TEST_CHECK_INT(at_rest.vertex_count - plain.vertex_count,
				   under.vertex_count);
		VOE_TEST_CHECK_INT(at_rest.index_count - plain.index_count,
				   under.index_count);
	}
}

// However the eye is placed there are the same triangles: one down an axis, one
// off in the corner, and one so nearly along an arrow that the direction across
// that arrow has to be taken from somewhere else.
static void the_count_is_the_same_from_anywhere(voe_base_arena *arena)
{
	const voe_math_float3 EYES[4] = { { 1.0f, 2.0f, 13.0f },
					  { 11.0f, 2.0f, 3.0f },
					  { 1.0f, 12.0f, 3.0f },
					  { -6.0f, -3.0f, -7.0f } };
	uint32_t indices = 0;
	int which;

	for (which = 0; which < 4; which++) {
		voe_3d_gizmo_mesh plain = { 0 };
		voe_3d_gizmo_mesh under = { 0 };

		VOE_TEST_CHECK(voe_3d_gizmo_quads(seen_from(EYES[which]),
						  VOE_3D_GIZMO_XY, arena,
						  &plain, &under));
		if (which == 0)
			indices = plain.index_count + under.index_count;
		VOE_TEST_CHECK_INT(plain.index_count + under.index_count,
				   (int)indices);
	}
	VOE_TEST_CHECK_INT((int)indices, VOE_3D_GIZMO_INDICES);
}

// A gizmo of no size is built from nothing and leaves both meshes as they were.
static void no_shaft_builds_nothing(voe_base_arena *arena)
{
	voe_3d_gizmo gizmo = seen_from((voe_math_float3){ 4.0f, 6.0f, 9.0f });
	voe_3d_gizmo_mesh plain = { .vertex_count = 9 };
	voe_3d_gizmo_mesh under = { .vertex_count = 9 };

	gizmo.shaft = 0.0f;
	VOE_TEST_CHECK(!voe_3d_gizmo_quads(gizmo, VOE_3D_GIZMO_NONE, arena,
					   &plain, &under));
	VOE_TEST_CHECK_INT(plain.vertex_count, 9);
	VOE_TEST_CHECK_INT(under.vertex_count, 9);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	each_arrow_is_hit_across_its_middle();
	each_square_is_hit_through_its_middle();
	empty_space_hits_nothing();
	twice_as_far_is_twice_the_shaft();
	an_unseen_gizmo_has_no_shaft(arena);
	an_axis_grab_keeps_the_other_two();
	a_plane_grab_keeps_its_normal();
	a_ray_in_a_plane_is_refused();
	the_two_meshes_fit_the_constants(arena);
	every_triangle_faces_the_eye(arena);
	marking_moves_one_handle_across(arena);
	the_count_is_the_same_from_anywhere(arena);
	no_shaft_builds_nothing(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
