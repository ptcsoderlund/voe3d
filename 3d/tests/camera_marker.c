// The camera marker: how much geometry it builds, that a camera scaled to
// nothing has none and cannot be picked, and where a ray meets its box square
// on, turned, and not at all. Needs no graphics card: the marker is arithmetic
// into an arena.
#include <3d/camera_marker.h>
#include <3d/projection.h>

#include <base/arena.h>

#include <testing/test.h>

#include <math.h>

#define SCRATCH (1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

static voe_scene_camera the_lens(void)
{
	return (voe_scene_camera){
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
}

// A camera at `position`, turned `turn` radians about Y, at `scale`.
static voe_scene_transform a_pose(voe_math_float3 position, float turn,
				  float scale)
{
	return (voe_scene_transform){
		.position = voe_math_double3_from_float3(position),
		.rotation = { 0.0f, sinf(turn * 0.5f), 0.0f,
			      cosf(turn * 0.5f) },
		.scale = { scale, scale, scale },
	};
}

// Where the_view's eye stands, which its quads are about.
static const voe_math_double3 EYE = { 0.0, 0.0, 5.0 };

// Five metres back along +Z, looking down -Z.
static voe_render_view the_view(void)
{
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(a_pose((voe_math_float3){ 0.0f, 0.0f, 5.0f },
					  0.0f, 1.0f),
				   the_lens(), (float)WIDTH / (float)HEIGHT,
				   &view));
	return view;
}

static voe_3d_ray down_minus_z_from(float x)
{
	return (voe_3d_ray){ .origin = { x, 0.0f, 5.0f },
			     .direction = { 0.0f, 0.0f, -1.0f } };
}

// Seen from (0, 0, 5) no edge of the marker is end-on, so every one of the
// twenty is a quad.
static void an_identity_pose_builds_twenty_edges(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_camera_marker_quads(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f),
		the_lens(), the_view(), EYE, (voe_platform_size){ WIDTH, HEIGHT },
		2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, VOE_3D_CAMERA_MARKER_VERTICES);
	VOE_TEST_CHECK_INT(mesh.index_count, VOE_3D_CAMERA_MARKER_INDICES);
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
}

static void a_pose_scaled_to_nothing_builds_nothing_and_is_not_hit(
	voe_base_arena *arena)
{
	voe_scene_transform nothing =
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f);
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(!voe_3d_camera_marker_quads(
		nothing, the_lens(), the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 0);
	VOE_TEST_CHECK(
		!voe_3d_camera_marker_hit(nothing, down_minus_z_from(0.0f), NULL));
}

// The box's near face is its z half extent, 0.15 m, in front of the origin.
static void a_ray_hits_the_box_square_on(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_camera_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f),
		down_minus_z_from(0.0f), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 4.85f, 1e-4f);
}

// Turned a quarter about Y the box's x half extent, 0.1 m, faces the ray.
static void a_ray_hits_the_turned_box_on_its_side(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_camera_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 1.5707964f, 1.0f),
		down_minus_z_from(0.0f), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 4.9f, 1e-4f);
}

// A metre to the side passes the box by, and the frustum's lines it crosses
// are never hit.
static void a_ray_beside_the_box_misses(void)
{
	VOE_TEST_CHECK(!voe_3d_camera_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f),
		down_minus_z_from(1.0f), NULL));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	an_identity_pose_builds_twenty_edges(arena);
	a_pose_scaled_to_nothing_builds_nothing_and_is_not_hit(arena);
	a_ray_hits_the_box_square_on();
	a_ray_hits_the_turned_box_on_its_side();
	a_ray_beside_the_box_misses();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
