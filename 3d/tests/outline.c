// A selected shape's silhouette as quads: how many edges a cube has from where,
// how wide the quads are in metres, which way they face and what is not
// outlined at all.
//
// IT NEEDS NO GRAPHICS CARD. The silhouette is a walk over edges the CPU holds
// and two matrices built from a camera, and the answer is arrays in an arena, so
// every claim here is checkable on a build box with no Vulkan. What only a drawn
// frame can say — that the line is the width it was asked for on the picture —
// belongs to the card that draws it.
#include <3d/outline.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <render/device.h>

#include <scene/camera_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>

// The three shapes' triangles and edges are a couple of hundred kilobytes
// (3d/shape_geometry.h), and every case below builds a capped outline into the
// same arena.
#define SCRATCH (4 * 1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// Two pixels of line, which is what a selection is drawn with.
#define PIXELS 2.0f

// Five metres back along +Z, looking down -Z, sixty degrees vertically. A cube a
// metre across at the origin (ADR-0191) has its near face at z = 0.5, four and a
// half metres in front of the eye.
static voe_scene_camera the_camera(void)
{
	return (voe_scene_camera){
		.eye = { 0.0f, 0.0f, 5.0f },
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
}

// The same two matrices the pass is opened with, and the eye beside them.
static voe_render_view the_view(voe_platform_size size)
{
	voe_scene_camera camera = the_camera();

	return (voe_render_view){
		.view = voe_scene_camera_view(camera),
		.projection = voe_3d_projection(
			camera, (float)size.width / (float)size.height),
		.eye = camera.eye,
	};
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 4,
		.intent_types = 4,
		.structure_requests = 4,
		.structure_bytes = 128,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_3d_shape_register(world, 8);
	return world;
}

// A shape of `kind` at (0, 0, z), turned `turn` radians about Y.
static voe_ecs_entity a_shape(voe_ecs_world *world, uint32_t kind, float z,
			      float turn)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = { 0.0f, 0.0f, z },
		.rotation = { 0.0f, sinf(turn * 0.5f), 0.0f,
			      cosf(turn * 0.5f) },
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	VOE_TEST_CHECK(voe_3d_shape_add(world, entity,
					(voe_3d_shape){
						.kind = kind,
						.colour = VOE_3D_SHAPE_GREY }));
	return entity;
}

static voe_3d_outlined outlining(voe_ecs_entity entity,
				 const voe_3d_shape_geometries *geometries)
{
	return (voe_3d_outlined){
		.entity = entity,
		.geometries = geometries,
		.colour = { 1.0f, 1.0f, 1.0f },
		.pixels = PIXELS,
		.size = { WIDTH, HEIGHT },
	};
}

// A cube seen square on shows four of its twelve folds: the four edges of the
// face that is towards the eye, where it meets the four faces that are not. The
// quads stand just outside those edges, so every corner of them is at the half
// metre the cube is and never inside it, and none is further out than the
// corner-to-corner distance a rotated cube would reach.
static void a_cube_seen_square_on_has_four_edges(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity cube = a_shape(world, VOE_3D_SHAPE_CUBE, 0.0f, 0.0f);
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_outline_quads(world, outlining(cube, geometries),
					    the_view((voe_platform_size){
						    WIDTH, HEIGHT }),
					    arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 16);
	VOE_TEST_CHECK_INT(mesh.index_count, 24);

	for (uint32_t i = 0; i < mesh.vertex_count; i++) {
		float x = fabsf(mesh.vertices[i].position.x);
		float y = fabsf(mesh.vertices[i].position.y);

		VOE_TEST_CHECK(x <= 0.71f && y <= 0.71f);
		// Outside the face it hugs, never inside the cube.
		VOE_TEST_CHECK((x > y ? x : y) >= 0.5f - 1e-3f);
	}

	// Every index names a vertex that exists.
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
}

// Turned forty-five degrees about Y the same cube shows two faces, and the
// silhouette is the six edges around them.
static void a_turned_cube_has_six(voe_base_arena *arena,
				  const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity cube =
		a_shape(world, VOE_3D_SHAPE_CUBE, 0.0f, 0.7853982f);
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_outline_quads(world, outlining(cube, geometries),
					    the_view((voe_platform_size){
						    WIDTH, HEIGHT }),
					    arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 24);
	VOE_TEST_CHECK_INT(mesh.index_count, 36);
}

// The width of one quad, in metres: its first corner to the corner that was
// moved outwards from it.
static float first_quad_width(voe_3d_outline_mesh mesh)
{
	return voe_math_float3_length(
		voe_math_float3_sub(mesh.vertices[2].position,
				    mesh.vertices[0].position));
}

// Twice as far away is twice as wide in metres, which is what the same width on
// the picture means. The near face is four and a half metres in front of the eye
// at the origin and nine metres in front of it four and a half metres further
// back.
static void twice_as_far_is_twice_as_wide(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_3d_outline_mesh near_mesh = { 0 };
	voe_3d_outline_mesh far_mesh = { 0 };
	voe_ecs_world *near_world = a_world(arena);
	voe_ecs_world *far_world = a_world(arena);
	voe_ecs_entity near_cube =
		a_shape(near_world, VOE_3D_SHAPE_CUBE, 0.0f, 0.0f);
	voe_ecs_entity far_cube =
		a_shape(far_world, VOE_3D_SHAPE_CUBE, -4.5f, 0.0f);

	VOE_TEST_CHECK(voe_3d_outline_quads(near_world,
					    outlining(near_cube, geometries),
					    the_view(size), arena, &near_mesh));
	VOE_TEST_CHECK(voe_3d_outline_quads(far_world,
					    outlining(far_cube, geometries),
					    the_view(size), arena, &far_mesh));
	VOE_TEST_CHECK_FLOAT(first_quad_width(far_mesh),
			     2.0f * first_quad_width(near_mesh), 1e-5f);
	// And that width is the two pixels it was asked for: at four and a half
	// metres, the height of the picture covers 2 * 4.5 * tan(30 degrees)
	// metres over four hundred and eighty pixels.
	VOE_TEST_CHECK_FLOAT(first_quad_width(near_mesh),
			     PIXELS * 2.0f * 4.5f * 0.5773503f / (float)HEIGHT,
			     1e-6f);
}

// Every triangle faces the eye, because the pipeline culls back faces: the
// normal worked out from its own three corners points at the eye and not away
// from it.
static void every_triangle_faces_the_eye(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity cube =
		a_shape(world, VOE_3D_SHAPE_CUBE, 0.0f, 0.7853982f);
	voe_render_view view = the_view((voe_platform_size){ WIDTH, HEIGHT });
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_outline_quads(world, outlining(cube, geometries),
					    view, arena, &mesh));

	for (uint32_t i = 0; i + 2 < mesh.index_count; i += 3) {
		voe_math_float3 a = mesh.vertices[mesh.indices[i]].position;
		voe_math_float3 b = mesh.vertices[mesh.indices[i + 1]].position;
		voe_math_float3 c = mesh.vertices[mesh.indices[i + 2]].position;
		voe_math_float3 normal = voe_math_float3_cross(
			voe_math_float3_sub(b, a), voe_math_float3_sub(c, a));

		VOE_TEST_CHECK(voe_math_float3_dot(
				       normal,
				       voe_math_float3_sub(view.eye, a)) > 0.0f);
	}
}

// A capsule's silhouette is many small edges rather than four long ones, and the
// cap is what keeps it off the end of a frame's transient pool.
static void a_capsule_has_many_and_no_more_than_the_cap(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity capsule =
		a_shape(world, VOE_3D_SHAPE_CAPSULE, 0.0f, 0.0f);
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_outline_quads(world,
					    outlining(capsule, geometries),
					    the_view((voe_platform_size){
						    WIDTH, HEIGHT }),
					    arena, &mesh));
	VOE_TEST_CHECK(mesh.vertex_count > 4 * 4);
	VOE_TEST_CHECK(mesh.vertex_count <= VOE_3D_OUTLINE_VERTICES);
	VOE_TEST_CHECK_INT(mesh.index_count, mesh.vertex_count / 4 * 6);
}

// Nothing to outline is false and an untouched answer, every time: a selection
// that is nothing, an entity that draws nothing, one that is nowhere, a kind
// this build does not know, and no store at all.
static void nothing_to_outline_is_false(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_render_view view = the_view((voe_platform_size){ WIDTH, HEIGHT });
	voe_ecs_entity nothing = { 0 };
	voe_ecs_entity bare = { 0 };
	voe_ecs_entity nowhere = { 0 };
	voe_ecs_entity unknown;
	voe_3d_outline_mesh mesh = { 0 };
	voe_render_vertex sentinel = { 0 };

	mesh.vertices = &sentinel;
	mesh.vertex_count = 7;

	VOE_TEST_CHECK(!voe_3d_outline_quads(
		world, outlining(nothing, geometries), view, arena, &mesh));

	// A transform and no shape.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, bare,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(!voe_3d_outline_quads(world, outlining(bare, geometries),
					     view, arena, &mesh));

	// A shape and no transform.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &nowhere));
	VOE_TEST_CHECK(voe_3d_shape_add(world, nowhere,
					(voe_3d_shape){
						.kind = VOE_3D_SHAPE_CUBE,
						.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(!voe_3d_outline_quads(
		world, outlining(nowhere, geometries), view, arena, &mesh));

	// A kind nothing in this build draws.
	unknown = a_shape(world, 99, 0.0f, 0.0f);
	VOE_TEST_CHECK(!voe_3d_outline_quads(
		world, outlining(unknown, geometries), view, arena, &mesh));

	// And a caller with no store.
	VOE_TEST_CHECK(!voe_3d_outline_quads(world, outlining(unknown, NULL),
					     view, arena, &mesh));

	VOE_TEST_CHECK(mesh.vertices == &sentinel);
	VOE_TEST_CHECK_INT(mesh.vertex_count, 7);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	a_cube_seen_square_on_has_four_edges(arena, &geometries);
	a_turned_cube_has_six(arena, &geometries);
	twice_as_far_is_twice_as_wide(arena, &geometries);
	every_triangle_faces_the_eye(arena, &geometries);
	a_capsule_has_many_and_no_more_than_the_cap(arena, &geometries);
	nothing_to_outline_is_false(arena, &geometries);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
