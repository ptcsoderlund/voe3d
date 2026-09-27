// The rotate gizmo's rings: which ring a ray meets, the angle a ray points at
// about a ring's axis, and the triangles the rings are drawn as.
//
// NONE OF IT NEEDS A GRAPHICS CARD: a gizmo is one position, one eye and three
// rings of plain geometry (3d/gizmo_rings.h), so every claim is a ray or an eye
// in and numbers out, the triangles counted and their winding measured here.
//
// The gizmo stands away from the world's origin on purpose, as 3d/tests/gizmo.c
// says: a ring test that passes at (0, 0, 0) can be one that forgot the origin.
#include <3d/gizmo.h>
#include <3d/gizmo_rings.h>
#include <3d/pick.h>

#include <base/arena.h>

#include <math/double3.h>
#include <math/float3.h>

#include <testing/test.h>

// A shaft of one metre, so a ring's radius reads as one here.
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

static voe_3d_gizmo a_gizmo(voe_math_double3 origin)
{
	return (voe_3d_gizmo){
		.origin = origin,
		.eye = { origin.x + 4.0, origin.y + 6.0, origin.z + 9.0 },
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

// A ray square on to each ring's plane through a point of its rim meets that
// ring, and the other two rings' planes run along it and are met by nothing.
static void each_ring_is_hit_on_its_rim(void)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE);
	int axis;

	for (axis = 0; axis < 3; axis++)
		VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
					   gizmo, aimed_at(gizmo,
							   AXIS[(axis + 1) % 3],
							   AXIS[axis])),
				   VOE_3D_GIZMO_X + axis);
}

// A ray through the centre crosses all three planes a whole radius from every
// rim.
static void the_centre_hits_nothing(void)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE);
	voe_math_float3 diagonal =
		voe_math_float3_normalize((voe_math_float3){ 1.0f, 1.0f, 1.0f });

	VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
				   gizmo, aimed_at(gizmo,
						   (voe_math_float3){ 0 },
						   diagonal)),
			   VOE_3D_GIZMO_NONE);
}

// A ray through a point of the X ring and a point of the Y ring meets the one
// it reaches first, from whichever end it comes.
static void the_nearer_ring_wins(void)
{
	voe_3d_gizmo gizmo = a_gizmo(WHERE);
	voe_math_float3 on_x = { 0.0f, 0.8660254f, 0.5f };
	voe_math_float3 on_y = { 0.5f, 0.0f, 0.8660254f };
	voe_math_float3 x_to_y =
		voe_math_float3_normalize(voe_math_float3_sub(on_y, on_x));

	VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
				   gizmo, aimed_at(gizmo, on_x,
						   voe_math_float3_neg(x_to_y))),
			   VOE_3D_GIZMO_X);
	VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
				   gizmo, aimed_at(gizmo, on_y, x_to_y)),
			   VOE_3D_GIZMO_Y);
}

// The pointer moved from +Z to +X of the Y ring, seen from above, is a quarter
// turn about Y, right-handed: that is what a drag of it reads, near the origin
// and 100 km out alike.
static void a_quarter_turn_about_y_reads_a_quarter(void)
{
	const voe_math_double3 WHERES[2] = { WHERE, FAR };
	int which;

	for (which = 0; which < 2; which++) {
		voe_3d_gizmo gizmo = a_gizmo(WHERES[which]);
		float from = 9.0f;
		float to = 9.0f;

		VOE_TEST_CHECK(voe_3d_gizmo_rings_angle(
			gizmo, VOE_3D_GIZMO_Y,
			aimed_at(gizmo, (voe_math_float3){ 0.0f, 0.0f, 0.6f },
				 AXIS[1]),
			&from));
		VOE_TEST_CHECK(voe_3d_gizmo_rings_angle(
			gizmo, VOE_3D_GIZMO_Y,
			aimed_at(gizmo, (voe_math_float3){ 0.6f, 0.0f, 0.0f },
				 AXIS[1]),
			&to));
		VOE_TEST_CHECK_FLOAT(to - from, HALF_PI, 1e-4f);
		VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
					   gizmo, aimed_at(gizmo, AXIS[0],
							   AXIS[1])),
				   VOE_3D_GIZMO_Y);
	}
}

// A ray lying in the Y ring's plane crosses it nowhere, and neither nothing nor
// a plane handle has an angle: all three refuse and write nothing, near the
// origin and 100 km out.
static void a_ray_in_the_plane_is_refused(void)
{
	const voe_math_double3 WHERES[2] = { WHERE, FAR };
	int which;

	for (which = 0; which < 2; which++) {
		voe_3d_gizmo gizmo = a_gizmo(WHERES[which]);
		voe_3d_ray along = aimed_at(gizmo, AXIS[2], AXIS[0]);
		voe_3d_ray down = aimed_at(gizmo, AXIS[2], AXIS[1]);
		float untouched = 9.0f;

		VOE_TEST_CHECK(!voe_3d_gizmo_rings_angle(gizmo, VOE_3D_GIZMO_Y,
							 along, &untouched));
		VOE_TEST_CHECK(!voe_3d_gizmo_rings_angle(
			gizmo, VOE_3D_GIZMO_NONE, down, &untouched));
		VOE_TEST_CHECK(!voe_3d_gizmo_rings_angle(gizmo, VOE_3D_GIZMO_ZX,
							 down, &untouched));
		VOE_TEST_CHECK_FLOAT(untouched, 9.0f, 0.0f);
	}
}

// Whatever is marked, the two meshes together are the header's counts, and
// the marked ring is one ring's worth moved out of the plain mesh.
static void marking_moves_one_ring_across(voe_base_arena *arena)
{
	const uint32_t RING_VERTICES = VOE_3D_GIZMO_RING_SEGMENTS * 4;
	const uint32_t RING_INDICES = VOE_3D_GIZMO_RING_SEGMENTS * 6;
	voe_3d_gizmo gizmo = a_gizmo(WHERE);
	int marked;

	for (marked = VOE_3D_GIZMO_NONE; marked <= VOE_3D_GIZMO_Z; marked++) {
		voe_3d_gizmo_mesh plain = { 0 };
		voe_3d_gizmo_mesh under = { 0 };
		uint32_t moved = marked == VOE_3D_GIZMO_NONE ? 0 : 1;

		VOE_TEST_CHECK(voe_3d_gizmo_rings_quads(
			gizmo, (voe_3d_gizmo_handle)marked, arena, &plain,
			&under));
		VOE_TEST_CHECK_INT(plain.vertex_count + under.vertex_count,
				   VOE_3D_GIZMO_RING_VERTICES);
		VOE_TEST_CHECK_INT(plain.index_count + under.index_count,
				   VOE_3D_GIZMO_RING_INDICES);
		VOE_TEST_CHECK_INT(under.vertex_count, moved * RING_VERTICES);
		VOE_TEST_CHECK_INT(under.index_count, moved * RING_INDICES);
	}
}

// Every triangle of both meshes faces the eye, which is at nought in their
// space, from two eyes on either side of all three planes, and 100 km out.
static void every_triangle_faces_the_eye(voe_base_arena *arena)
{
	const voe_math_double3 WHERES[3] = { WHERE, WHERE, FAR };
	const voe_math_double3 LOOK[3] = { { 4.0, 6.0, 9.0 },
					   { -5.0, -7.0, -3.0 },
					   { 3.0, -8.0, 5.0 } };
	int which;

	for (which = 0; which < 3; which++) {
		voe_3d_gizmo gizmo = a_gizmo(WHERES[which]);
		voe_3d_gizmo_mesh mesh[2] = { { 0 }, { 0 } };
		int side;

		gizmo.eye = voe_math_double3_add(gizmo.origin, LOOK[which]);
		VOE_TEST_CHECK(voe_3d_gizmo_rings_quads(
			gizmo, VOE_3D_GIZMO_Z, arena, &mesh[0], &mesh[1]));
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
	voe_3d_gizmo gizmo = a_gizmo(WHERE);
	voe_3d_gizmo_mesh plain = { .vertex_count = 9 };
	voe_3d_gizmo_mesh under = { .vertex_count = 9 };

	gizmo.shaft = 0.0f;
	VOE_TEST_CHECK_INT(voe_3d_gizmo_rings_hit(
				   gizmo, aimed_at(gizmo, AXIS[1], AXIS[0])),
			   VOE_3D_GIZMO_NONE);
	VOE_TEST_CHECK(!voe_3d_gizmo_rings_quads(gizmo, VOE_3D_GIZMO_NONE,
						 arena, &plain, &under));
	VOE_TEST_CHECK_INT(plain.vertex_count, 9);
	VOE_TEST_CHECK_INT(under.vertex_count, 9);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	each_ring_is_hit_on_its_rim();
	the_centre_hits_nothing();
	the_nearer_ring_wins();
	a_quarter_turn_about_y_reads_a_quarter();
	a_ray_in_the_plane_is_refused();
	marking_moves_one_ring_across(arena);
	every_triangle_faces_the_eye(arena);
	no_shaft_is_nothing(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
