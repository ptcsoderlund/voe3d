// The sun marker: how many edges it builds, that a turned sun's arrow ends a
// metre along its turned -Z, that scale changes nothing, and where a ray meets
// its cube square on, turned, and not at all. Needs no graphics card: the
// marker is arithmetic into an arena.
#include <3d/projection.h>
#include <3d/sun_marker.h>

#include <base/arena.h>

#include <scene/camera_component.h>

#include <testing/test.h>

#include <math.h>

#define SCRATCH (1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// A pose at `position`, turned `turn` radians about Y, at `scale`.
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

	VOE_TEST_CHECK(voe_3d_view(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 5.0f }, 0.0f, 1.0f),
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f },
		(float)WIDTH / (float)HEIGHT, &view));
	return view;
}

static voe_3d_ray down_minus_z_from(float x)
{
	return (voe_3d_ray){ .origin = { x, 0.0f, 5.0f },
			     .direction = { 0.0f, 0.0f, -1.0f } };
}

// A sun at the origin turned a quarter about Y shines along -X, so from
// (0, 0, 5) no edge is end-on.
static voe_scene_transform turned_sun(float scale)
{
	return a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 1.5707964f, scale);
}

static void a_turned_sun_builds_every_edge(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_sun_marker_quads(
		turned_sun(1.0f), the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(VOE_3D_SUN_MARKER_EDGES, 24 + 1 + 4);
	VOE_TEST_CHECK_INT(mesh.vertex_count, VOE_3D_SUN_MARKER_VERTICES);
	VOE_TEST_CHECK_INT(mesh.index_count, VOE_3D_SUN_MARKER_INDICES);
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
}

// The shaft is the edge after the circle; its far end's two corners straddle
// the tip, pushed on past it by half a width, which a thousandth of a pixel
// makes nothing. About EYE, the tip at (-1, 0, 0) is (-1, 0, -5).
static void a_turned_sun_s_arrow_ends_a_metre_along_its_minus_z(
	voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };
	uint32_t v = VOE_3D_SUN_MARKER_CIRCLE * 4;

	VOE_TEST_CHECK(voe_3d_sun_marker_quads(
		turned_sun(1.0f), the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 0.001f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, VOE_3D_SUN_MARKER_VERTICES);
	VOE_TEST_CHECK_FLOAT((mesh.vertices[v + 1].position.x +
			      mesh.vertices[v + 3].position.x) * 0.5f,
			     -1.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT((mesh.vertices[v + 1].position.y +
			      mesh.vertices[v + 3].position.y) * 0.5f,
			     0.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT((mesh.vertices[v + 1].position.z +
			      mesh.vertices[v + 3].position.z) * 0.5f,
			     -5.0f, 1e-3f);
}

// Scale is ignored: a sun scaled up, or to nothing, builds the same quads.
static void a_scaled_sun_builds_the_same_quads(voe_base_arena *arena)
{
	const float scales[] = { 3.0f, 0.0f };
	voe_3d_outline_mesh plain = { 0 };

	VOE_TEST_CHECK(voe_3d_sun_marker_quads(
		turned_sun(1.0f), the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &plain));
	for (uint32_t s = 0; s < 2; s++) {
		voe_3d_outline_mesh scaled = { 0 };

		VOE_TEST_CHECK(voe_3d_sun_marker_quads(
			turned_sun(scales[s]), the_view(), EYE,
			(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena,
			&scaled));
		VOE_TEST_CHECK_INT(scaled.vertex_count, plain.vertex_count);
		for (uint32_t i = 0; i < plain.vertex_count; i++) {
			VOE_TEST_CHECK_FLOAT(scaled.vertices[i].position.x,
					     plain.vertices[i].position.x, 1e-6f);
			VOE_TEST_CHECK_FLOAT(scaled.vertices[i].position.y,
					     plain.vertices[i].position.y, 1e-6f);
			VOE_TEST_CHECK_FLOAT(scaled.vertices[i].position.z,
					     plain.vertices[i].position.z, 1e-6f);
		}
	}
}

static void a_picture_with_no_area_builds_nothing(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(!voe_3d_sun_marker_quads(turned_sun(1.0f), the_view(),
						EYE, (voe_platform_size){ 0, 0 },
						2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 0);
}

// The cube's near face is its half extent, 0.25 m, in front of the origin —
// scaled to nothing or not.
static void a_ray_hits_the_cube_square_on(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_sun_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f),
		down_minus_z_from(0.0f), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 4.75f, 1e-4f);
	distance = 0.0f;
	VOE_TEST_CHECK(voe_3d_sun_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f),
		down_minus_z_from(0.0f), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 4.75f, 1e-4f);
}

// Turned an eighth about Y an edge of the cube faces the ray, 0.25 m times
// root two in front of the origin.
static void a_ray_hits_the_turned_cube_on_its_edge(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_sun_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.7853982f, 1.0f),
		down_minus_z_from(0.0f), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 5.0f - 0.25f * sqrtf(2.0f), 1e-4f);
}

// Half a metre to the side passes the cube by, though it crosses the circle's
// plane beside the arrow.
static void a_ray_beside_the_cube_misses(void)
{
	VOE_TEST_CHECK(!voe_3d_sun_marker_hit(
		a_pose((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f),
		down_minus_z_from(0.5f), NULL));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_turned_sun_builds_every_edge(arena);
	a_turned_sun_s_arrow_ends_a_metre_along_its_minus_z(arena);
	a_scaled_sun_builds_the_same_quads(arena);
	a_picture_with_no_area_builds_nothing(arena);
	a_ray_hits_the_cube_square_on();
	a_ray_hits_the_turned_cube_on_its_edge();
	a_ray_beside_the_cube_misses();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
