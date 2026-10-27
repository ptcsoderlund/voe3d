// The scale gizmo's arithmetic: which handle a ray meets on the entity's own
// axes, the factor a drag comes to, and the triangles it is drawn as.
//
// NONE OF IT NEEDS A GRAPHICS CARD: a gizmo is one position, one eye, one turn
// and four handles of plain geometry (3d/gizmo_scales.h), so every claim is a
// ray or an eye in and numbers out.
//
// The gizmo stands away from the world's origin on purpose, as 3d/tests/gizmo.c
// says: a test that passes at (0, 0, 0) can be one that forgot the origin.
#include <3d/gizmo.h>
#include <3d/gizmo_scales.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <testing/test.h>

// A shaft of one metre, so every fraction in the header reads as itself here.
#define SHAFT 1.0f

// How far back each ray starts from the point it is aimed at.
#define REACH 5.0f

// Enough for the four arrays one build pushes, many times over.
#define SCRATCH (1024 * 1024)

#define HALF_PI 1.57079632679489661923f

static const voe_math_double3 WHERE = { 1.0, 2.0, 3.0 };

// A gizmo 100 km out, for the claims that must read the same there.
static const voe_math_double3 FAR = { 100000.0, 2.0, 100000.0 };

static const voe_math_float3 AXIS[3] = { { 1.0f, 0.0f, 0.0f },
					 { 0.0f, 1.0f, 0.0f },
					 { 0.0f, 0.0f, 1.0f } };

// Where the eye stands from the gizmo, off every axis.
static const voe_math_float3 LOOK = { 4.0f, 6.0f, 9.0f };

static voe_math_quat no_turn(void)
{
	return voe_math_quat_from_axis_angle(AXIS[1], 0.0f);
}

static voe_3d_gizmo a_gizmo(voe_math_double3 origin, voe_math_float3 look)
{
	return (voe_3d_gizmo){
		.origin = origin,
		.eye = voe_math_double3_add(origin,
					    voe_math_double3_from_float3(look)),
		.shaft = SHAFT,
	};
}

// A ray at `offset` from the gizmo's origin, from REACH metres back along
// `from`, a unit vector.
static voe_3d_ray aimed_at(voe_3d_gizmo gizmo, voe_math_float3 offset,
			   voe_math_float3 from)
{
	voe_math_float3 start =
		voe_math_float3_add(offset, voe_math_float3_scale(from, REACH));

	return (voe_3d_ray){
		.origin = voe_math_double3_add(
			gizmo.origin, voe_math_double3_from_float3(start)),
		.direction = voe_math_float3_neg(from),
	};
}

// From the eye, a ray onto each tip meets that axis and one onto the centre
// meets the centre; beside a tip, and on each axis's far side of the centre,
// nothing.
static void each_tip_and_the_centre_is_hit(void)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE, LOOK);
	voe_math_float3 from = voe_math_float3_normalize(LOOK);
	int axis;

	VOE_TEST_CHECK_INT(voe_3d_gizmo_scales_hit(
				   gizmo, no_turn(),
				   aimed_at(gizmo, (voe_math_float3){ 0 }, from)),
			   VOE_3D_GIZMO_CENTRE);
	for (axis = 0; axis < 3; axis++) {
		voe_math_float3 beside = voe_math_float3_add(
			AXIS[axis],
			voe_math_float3_scale(
				voe_math_float3_normalize(voe_math_float3_cross(
					AXIS[axis], from)),
				0.3f));

		VOE_TEST_CHECK_INT(voe_3d_gizmo_scales_hit(
					   gizmo, no_turn(),
					   aimed_at(gizmo, AXIS[axis], from)),
				   VOE_3D_GIZMO_X + axis);
		VOE_TEST_CHECK_INT(voe_3d_gizmo_scales_hit(
					   gizmo, no_turn(),
					   aimed_at(gizmo, beside, from)),
				   VOE_3D_GIZMO_NONE);
		VOE_TEST_CHECK_INT(
			voe_3d_gizmo_scales_hit(
				gizmo, no_turn(),
				aimed_at(gizmo,
					 voe_math_float3_scale(AXIS[axis],
							       -0.3f),
					 from)),
			VOE_3D_GIZMO_NONE);
	}
}

// A quarter turn about Y takes the entity's X onto the world's -Z: X's tip is
// there, and the world's +X is Z's, near the origin and 100 km out.
static void a_quarter_turn_moves_the_tips(void)
{
	const voe_math_double3 WHERES[2] = { WHERE, FAR };
	voe_math_quat turn = voe_math_quat_from_axis_angle(AXIS[1], HALF_PI);
	voe_math_float3 from = voe_math_float3_normalize(LOOK);
	int which;

	for (which = 0; which < 2; which++) {
		voe_3d_gizmo gizmo = a_gizmo(WHERES[which], LOOK);

		VOE_TEST_CHECK_INT(
			voe_3d_gizmo_scales_hit(
				gizmo, turn,
				aimed_at(gizmo,
					 (voe_math_float3){ 0.0f, 0.0f, -1.0f },
					 from)),
			VOE_3D_GIZMO_X);
		VOE_TEST_CHECK_INT(voe_3d_gizmo_scales_hit(
					   gizmo, turn,
					   aimed_at(gizmo, AXIS[0], from)),
				   VOE_3D_GIZMO_Z);
	}
}

// The turned entity's X grabbed one shaft out and now two shafts out is twice
// the scale, and the grab lands on the axis itself.
static void an_axis_dragged_to_twice_reads_two(void)
{
	voe_math_quat turn = voe_math_quat_from_axis_angle(AXIS[1], HALF_PI);
	voe_3d_gizmo gizmo = a_gizmo(WHERE, LOOK);
	voe_math_double3 press = { 0 };
	voe_math_double3 now = { 0 };

	VOE_TEST_CHECK(voe_3d_gizmo_scales_grab(
		gizmo, turn, VOE_3D_GIZMO_X,
		aimed_at(gizmo, (voe_math_float3){ 0.0f, 0.0f, -1.0f },
			 AXIS[1]),
		&press));
	VOE_TEST_CHECK(voe_3d_gizmo_scales_grab(
		gizmo, turn, VOE_3D_GIZMO_X,
		aimed_at(gizmo, (voe_math_float3){ 0.3f, 0.0f, -2.0f },
			 AXIS[1]),
		&now));
	VOE_TEST_CHECK_FLOAT((float)(press.z - WHERE.z), -1.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT((float)(now.x - WHERE.x), 0.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(voe_3d_gizmo_scales_factor(gizmo, turn,
							VOE_3D_GIZMO_X, press,
							now),
			     2.0f, 1e-4f);
}

// The centre's factor after a drag of `up` metres up the picture, the eye
// `look` from the gizmo, which is straight along `look`'s own line.
static float centre_dragged(voe_math_float3 look, voe_math_float3 up)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE, look);
	voe_math_float3 from = voe_math_float3_normalize(look);
	voe_math_double3 press = { 0 };
	voe_math_double3 now = { 0 };

	VOE_TEST_CHECK(voe_3d_gizmo_scales_grab(
		gizmo, no_turn(), VOE_3D_GIZMO_CENTRE,
		aimed_at(gizmo, (voe_math_float3){ 0 }, from), &press));
	VOE_TEST_CHECK(voe_3d_gizmo_scales_grab(gizmo, no_turn(),
						VOE_3D_GIZMO_CENTRE,
						aimed_at(gizmo, up, from),
						&now));
	return voe_3d_gizmo_scales_factor(gizmo, no_turn(), VOE_3D_GIZMO_CENTRE,
					  press, now);
}

// Seen from in front, half a shaft up is half again and half a shaft down is
// half; five shafts down stops at the floor. Seen from straight above, up the
// picture is the world's -Z.
static void the_centre_grows_up_and_shrinks_down(void)
{
	const voe_math_float3 FRONT = { 0.0f, 0.0f, 10.0f };
	const voe_math_float3 ABOVE = { 0.0f, 10.0f, 0.0f };

	VOE_TEST_CHECK_FLOAT(
		centre_dragged(FRONT, (voe_math_float3){ 0.0f, 0.5f, 0.0f }),
		1.5f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(
		centre_dragged(FRONT, (voe_math_float3){ 0.0f, -0.5f, 0.0f }),
		0.5f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(
		centre_dragged(FRONT, (voe_math_float3){ 0.0f, -5.0f, 0.0f }),
		VOE_3D_GIZMO_SCALE_LEAST, 0.0f);
	VOE_TEST_CHECK_FLOAT(
		centre_dragged(ABOVE, (voe_math_float3){ 0.0f, 0.0f, -0.5f }),
		1.5f, 1e-4f);
}

// Whatever is marked, the two meshes together are the header's counts, the
// marked handle's quads moved out of the plain mesh, and every triangle faces
// the eye at nought.
static void the_quads_fill_the_constants(voe_base_arena *arena)
{
	const voe_3d_gizmo_handle MARKED[5] = {
		VOE_3D_GIZMO_NONE, VOE_3D_GIZMO_X, VOE_3D_GIZMO_Y,
		VOE_3D_GIZMO_Z, VOE_3D_GIZMO_CENTRE
	};
	const uint32_t MOVED[5] = { 0, 8, 8, 8, 4 };
	voe_math_quat turn = voe_math_quat_from_axis_angle(LOOK, 0.7f);
	voe_3d_gizmo gizmo = a_gizmo(WHERE, LOOK);
	int which;

	for (which = 0; which < 5; which++) {
		voe_3d_gizmo_mesh mesh[2] = { { 0 }, { 0 } };
		int side;

		VOE_TEST_CHECK(voe_3d_gizmo_scales_quads(
			gizmo, turn, MARKED[which], arena, &mesh[0], &mesh[1]));
		VOE_TEST_CHECK_INT(mesh[0].vertex_count + mesh[1].vertex_count,
				   VOE_3D_GIZMO_SCALE_VERTICES);
		VOE_TEST_CHECK_INT(mesh[0].index_count + mesh[1].index_count,
				   VOE_3D_GIZMO_SCALE_INDICES);
		VOE_TEST_CHECK_INT(mesh[1].vertex_count, MOVED[which]);
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

				VOE_TEST_CHECK(voe_math_float3_dot(
						       normal,
						       voe_math_float3_neg(a)) >
					       0.0f);
			}
	}
}

// A gizmo of no size meets nothing and builds nothing.
static void no_shaft_is_nothing(voe_base_arena *arena)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE, LOOK);
	voe_3d_gizmo_mesh plain = { .vertex_count = 9 };
	voe_3d_gizmo_mesh under = { .vertex_count = 9 };

	gizmo.shaft = 0.0f;
	VOE_TEST_CHECK_INT(voe_3d_gizmo_scales_hit(
				   gizmo, no_turn(),
				   aimed_at(gizmo, (voe_math_float3){ 0 },
					    voe_math_float3_normalize(LOOK))),
			   VOE_3D_GIZMO_NONE);
	VOE_TEST_CHECK(!voe_3d_gizmo_scales_quads(gizmo, no_turn(),
						  VOE_3D_GIZMO_NONE, arena,
						  &plain, &under));
	VOE_TEST_CHECK_INT(plain.vertex_count, 9);
	VOE_TEST_CHECK_INT(under.vertex_count, 9);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	each_tip_and_the_centre_is_hit();
	a_quarter_turn_moves_the_tips();
	an_axis_dragged_to_twice_reads_two();
	the_centre_grows_up_and_shrinks_down();
	the_quads_fill_the_constants(arena);
	no_shaft_is_nothing(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
