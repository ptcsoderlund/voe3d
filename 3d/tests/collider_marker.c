// A collider's lines: how many quads each kind builds, that a box's lines
// scale with its size, that far from the origin they are still about the eye,
// and the collider that fits each built-in shape. Needs no graphics card: the
// lines are arithmetic into an arena.
#include <3d/collider_marker.h>
#include <3d/projection.h>
#include <3d/shape_component.h>

#include <base/arena.h>

#include <physics/collider_component.h>

#include <testing/test.h>

#include <math.h>

#define SCRATCH (1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// Where the eye stands relative to the shape's centre: off every axis, so no
// segment is seen end-on.
static const voe_math_double3 OFFSET = { 1.0, 2.0, 5.0 };

static voe_math_double3 plus(voe_math_double3 a, voe_math_double3 b)
{
	return (voe_math_double3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

// Looking down -Z from `eye`.
static voe_render_view view_from(voe_math_double3 eye)
{
	voe_render_view view = { 0 };
	voe_scene_transform pose = {
		.position = eye,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	VOE_TEST_CHECK(voe_3d_view(pose, lens, (float)WIDTH / (float)HEIGHT,
				   &view));
	return view;
}

static voe_physics_shape a_shape(uint32_t kind, voe_math_double3 centre,
				 voe_math_float3 half)
{
	return (voe_physics_shape){
		.kind = kind,
		.centre = centre,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.half = half,
	};
}

// The shape's lines seen from OFFSET past its centre, `pixels` wide.
static voe_3d_outline_mesh lines_of(voe_physics_shape shape, float pixels,
				    voe_base_arena *arena)
{
	voe_math_double3 eye = plus(shape.centre, OFFSET);
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_collider_marker_quads(
		shape, view_from(eye), eye, (voe_platform_size){ WIDTH, HEIGHT },
		pixels, arena, &mesh));
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
	return mesh;
}

static void each_kind_builds_its_own_count(voe_base_arena *arena)
{
	voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_3d_outline_mesh box = lines_of(
		a_shape(VOE_PHYSICS_COLLIDER_BOX, origin,
			(voe_math_float3){ 0.5f, 0.5f, 0.5f }),
		2.0f, arena);
	voe_3d_outline_mesh sphere = lines_of(
		a_shape(VOE_PHYSICS_COLLIDER_SPHERE, origin,
			(voe_math_float3){ 0.5f, 0.0f, 0.0f }),
		2.0f, arena);
	voe_3d_outline_mesh capsule = lines_of(
		a_shape(VOE_PHYSICS_COLLIDER_CAPSULE, origin,
			(voe_math_float3){ 0.5f, 1.0f, 0.0f }),
		2.0f, arena);

	VOE_TEST_CHECK_INT(box.vertex_count, 12 * 4);
	VOE_TEST_CHECK_INT(box.index_count, 12 * 6);
	VOE_TEST_CHECK_INT(sphere.vertex_count, 3 * 24 * 4);
	VOE_TEST_CHECK_INT(capsule.vertex_count,
			   VOE_3D_COLLIDER_MARKER_VERTICES);
	VOE_TEST_CHECK_INT(capsule.index_count, VOE_3D_COLLIDER_MARKER_INDICES);
}

// The furthest any corner of the quads is from the shape's centre, which is
// at `centre` about the eye.
static float furthest(voe_3d_outline_mesh mesh, voe_math_float3 centre)
{
	float most = 0.0f;

	for (uint32_t i = 0; i < mesh.vertex_count; i++) {
		float d = voe_math_float3_length(
			voe_math_float3_sub(mesh.vertices[i].position, centre));

		most = d > most ? d : most;
	}
	return most;
}

// A box of twice the size — what a transform scaled 2 hands physics — has its
// corner edges twice as far out. Hairline widths keep the ends' extension out
// of the ratio.
static void a_box_scaled_two_is_twice_as_far_out(voe_base_arena *arena)
{
	voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_math_float3 centre = { -1.0f, -2.0f, -5.0f };
	float one = furthest(lines_of(a_shape(VOE_PHYSICS_COLLIDER_BOX, origin,
					      (voe_math_float3){ 0.5f, 0.5f,
								 0.5f }),
				      0.01f, arena),
			     centre);
	float two = furthest(lines_of(a_shape(VOE_PHYSICS_COLLIDER_BOX, origin,
					      (voe_math_float3){ 1.0f, 1.0f,
								 1.0f }),
				      0.01f, arena),
			     centre);

	VOE_TEST_CHECK_FLOAT(one, sqrtf(3.0f) * 0.5f, 1e-2f);
	VOE_TEST_CHECK_FLOAT(two, 2.0f * one, 2e-2f);
}

// 100 km out, every corner is within a few metres of the eye.
static void far_away_the_quads_are_about_the_eye(voe_base_arena *arena)
{
	voe_math_double3 far = { 100000.0, 0.0, -100000.0 };
	voe_3d_outline_mesh mesh = lines_of(
		a_shape(VOE_PHYSICS_COLLIDER_CAPSULE, far,
			(voe_math_float3){ 0.5f, 1.0f, 0.0f }),
		2.0f, arena);

	VOE_TEST_CHECK_INT(mesh.vertex_count, VOE_3D_COLLIDER_MARKER_VERTICES);
	for (uint32_t i = 0; i < mesh.vertex_count; i++)
		VOE_TEST_CHECK(voe_math_float3_length(
				       mesh.vertices[i].position) < 10.0f);
}

static void no_area_and_no_kind_build_nothing(voe_base_arena *arena)
{
	voe_math_double3 eye = OFFSET;
	voe_physics_shape box =
		a_shape(VOE_PHYSICS_COLLIDER_BOX, (voe_math_double3){ 0 },
			(voe_math_float3){ 0.5f, 0.5f, 0.5f });
	voe_physics_shape unknown = box;
	voe_3d_outline_mesh mesh = { 0 };

	unknown.kind = 99;
	VOE_TEST_CHECK(!voe_3d_collider_marker_quads(
		box, view_from(eye), eye, (voe_platform_size){ 0, HEIGHT }, 2.0f,
		arena, &mesh));
	VOE_TEST_CHECK(!voe_3d_collider_marker_quads(
		unknown, view_from(eye), eye,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 0);
}

static void check_collider(voe_physics_collider c, uint32_t kind, float x,
			   float y, float z)
{
	VOE_TEST_CHECK_INT(c.kind, kind);
	VOE_TEST_CHECK_FLOAT(c.size.x, x, 0.0f);
	VOE_TEST_CHECK_FLOAT(c.size.y, y, 0.0f);
	VOE_TEST_CHECK_FLOAT(c.size.z, z, 0.0f);
	VOE_TEST_CHECK(!c.trigger);
}

static void each_shape_has_the_collider_that_fits_it(void)
{
	check_collider(voe_3d_shape_collider(VOE_3D_SHAPE_CUBE),
		       VOE_PHYSICS_COLLIDER_BOX, 1.0f, 1.0f, 1.0f);
	check_collider(voe_3d_shape_collider(VOE_3D_SHAPE_CYLINDER),
		       VOE_PHYSICS_COLLIDER_BOX, 1.0f, 1.0f, 1.0f);
	check_collider(voe_3d_shape_collider(VOE_3D_SHAPE_CAPSULE),
		       VOE_PHYSICS_COLLIDER_CAPSULE, 1.0f, 2.0f, 1.0f);
	check_collider(voe_3d_shape_collider(99), VOE_PHYSICS_COLLIDER_BOX,
		       1.0f, 1.0f, 1.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	each_kind_builds_its_own_count(arena);
	a_box_scaled_two_is_twice_as_far_out(arena);
	far_away_the_quads_are_about_the_eye(arena);
	no_area_and_no_kind_build_nothing(arena);
	each_shape_has_the_collider_that_fits_it();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
