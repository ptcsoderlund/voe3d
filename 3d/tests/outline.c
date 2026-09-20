// A selected shape's silhouette as quads: how many edges a cube has from where,
// how wide the quads are in metres, which way they face and what is not
// outlined at all.
//
// THE FIRST HALF NEEDS NO GRAPHICS CARD. The silhouette is a walk over edges the
// CPU holds and two matrices built from a camera, and the answer is arrays in an
// arena, so every claim there is checkable on a build box with no Vulkan.
//
// AND THE SECOND HALF IS THE ONE CLAIM ONLY A DRAWN FRAME CAN MAKE: that the
// outline shows through whatever stands in front of the entity. Two cubes down
// the camera's line of sight, the near one hiding the far one entirely, drawn
// into a headless device's own target and read back (ADR-0177) — the outline's
// pixels are where the hidden cube is, which no arithmetic over the quads can
// say, because it is the depth clear before the draw and not the quads that puts
// them there. It skips without a card, exactly as 3d/tests/draw_system.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/outline.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
#include <3d/shape_system.h>

#include <base/arena.h>
#include <base/error.h>

#include <ecs/world.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <render/device.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

// The three shapes' triangles and edges are a couple of hundred kilobytes
// (3d/shape_geometry.h), every case below builds a capped outline into the same
// arena, and the drawn half reads three pictures back into it as well.
#define SCRATCH (8 * 1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// Two pixels of line, which is what a selection is drawn with.
#define PIXELS 2.0f

// The picture the drawn half works in. Small, because every pixel of it is
// walked three times, and four by three so that the cubes below are square on
// it.
#define DRAWN_WIDTH 320
#define DRAWN_HEIGHT 240

// Three pixels of line there, which is a line thick enough to count.
#define DRAWN_PIXELS 3.0f

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
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 4,
		.structure_bytes = 128,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_3d_shape_register(world, 8);
	// The drawn half runs the shape and the draw systems over one of these,
	// and every table a system walks has to be registered — including the
	// panel table it has no rows in (3d/draw_system.h).
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	voe_3d_panel_register(world, 16);
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

// What the drawn half outlines: the caller's own unlit record, a colour nothing
// else in the picture can be, and a line thick enough to count on the picture
// that is about to be read.
static voe_3d_outlined
outlining_drawn(voe_ecs_entity entity,
		const voe_3d_shape_geometries *geometries,
		voe_3d_material record)
{
	return (voe_3d_outlined){
		.entity = entity,
		.geometries = geometries,
		.material = record,
		// Linear (1, 0, 1), which an sRGB target hands back as the bytes
		// (255, 0, 255) exactly: nought and one are the two values the
		// curve leaves alone. The cubes are grey and lit, so their three
		// channels are alike and no pixel of them can be this.
		.colour = { 1.0f, 0.0f, 1.0f },
		.pixels = DRAWN_PIXELS,
		.size = { DRAWN_WIDTH, DRAWN_HEIGHT },
	};
}

// One frame of the world, drawn into `target` with `outlined` outlined, and the
// picture that came out.
static voe_render_picture a_drawn_frame(voe_ecs_world *world,
					voe_render_device *device,
					voe_base_arena *arena,
					voe_render_target target,
					voe_3d_outlined outlined)
{
	voe_platform_size size = { DRAWN_WIDTH, DRAWN_HEIGHT };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	// The answer comes back outlining nothing, and outlining something is a
	// write to that answer — which is the whole of the caller's side of this.
	VOE_TEST_CHECK_INT(frame.outlined.entity.index, 0);
	VOE_TEST_CHECK_INT(frame.outlined.entity.generation, 0);
	frame.outlined = outlined;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light };

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(device, target, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	VOE_TEST_CHECK(voe_render_target_read(device, target, arena, &picture,
					      &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// Where a picture's outline is: how many pixels carry the outline's colour, how
// far left the leftmost of them is, and whether any of them is inside the
// rectangle the near cube covers.
struct outline_pixels {
	uint32_t count;
	uint32_t leftmost;
	bool inside_the_near_cube;
};

static struct outline_pixels outline_pixels_of(voe_render_picture picture)
{
	struct outline_pixels found = { .leftmost = picture.width };
	// Half the square the cube at (0, 0, 2) covers, in pixels: half a metre
	// against the half-height the picture covers three metres in front of
	// the eye — 3 * tan(30 degrees) — and the same number horizontally,
	// because a pixel is as wide as it is tall. It is the cube's middle
	// plane rather than its near face, so the rectangle is inside what the
	// cube really covers.
	float half = 0.5f / (3.0f * 0.5773503f) * (float)picture.height / 2.0f;

	for (uint32_t y = 0; y < picture.height; y++) {
		for (uint32_t x = 0; x < picture.width; x++) {
			const uint8_t *pixel =
				picture.pixels +
				((size_t)y * picture.width + x) * 4;

			if (pixel[0] != 255 || pixel[1] != 0 || pixel[2] != 255)
				continue;

			found.count++;
			if (x < found.leftmost)
				found.leftmost = x;
			if (fabsf((float)x - (float)picture.width / 2.0f) <=
				    half &&
			    fabsf((float)y - (float)picture.height / 2.0f) <=
				    half)
				found.inside_the_near_cube = true;
		}
	}
	return found;
}

// THE OUTLINE IS DRAWN, AND IT SHOWS THROUGH WHAT STANDS IN FRONT OF IT. A grey
// cube at the origin and a second one at (0, 0, 2), which is between it and the
// camera and covers it entirely. Three frames of that one world: one outlining
// nothing, one outlining the far cube and one outlining the near one.
static void the_outline_is_drawn_through_what_hides_it(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_platform_size size = { DRAWN_WIDTH, DRAWN_HEIGHT };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' own pools, one outline's worth of this frame's geometry,
	// and one object each for the two cubes and the outline — the three
	// transient numbers and the extra object a program that outlines
	// anything pays for (3d/draw_system.h).
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 3,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = VOE_3D_OUTLINE_VERTICES,
		.transient_indices = VOE_3D_OUTLINE_INDICES,
		.transient_geometries = 1,
		.passes = 1,
		.targets = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_render_target target = { 0 };
	voe_render_texture texture = { 0 };
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity eye = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_ecs_entity far_cube;
	voe_ecs_entity near_cube;
	struct outline_pixels nothing_outlined;
	struct outline_pixels far_outlined;
	struct outline_pixels near_outlined;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			return;
		}
		VOE_TEST_CHECK(device != NULL);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	VOE_TEST_CHECK(voe_render_target_create(device, DRAWN_WIDTH,
						DRAWN_HEIGHT, &target, &texture,
						&error));

	world = a_world(arena);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, the_camera()));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .direction = { 0.0f, -0.6f, -0.8f },
				   .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	far_cube = a_shape(world, VOE_3D_SHAPE_CUBE, 0.0f, 0.0f);
	near_cube = a_shape(world, VOE_3D_SHAPE_CUBE, 2.0f, 0.0f);
	voe_3d_shape_system_run(world, &shapes);

	nothing_outlined = outline_pixels_of(a_drawn_frame(
		world, device, arena, target, (voe_3d_outlined){ 0 }));
	far_outlined = outline_pixels_of(a_drawn_frame(
		world, device, arena, target,
		outlining_drawn(far_cube, geometries, shapes.outline)));
	near_outlined = outline_pixels_of(a_drawn_frame(
		world, device, arena, target,
		outlining_drawn(near_cube, geometries, shapes.outline)));

	// A frame that outlines nothing draws nothing in the outline's colour.
	VOE_TEST_CHECK_INT(nothing_outlined.count, 0);

	// The far cube outlined: a line of it, and some of that line is inside
	// the rectangle the near cube covers — which is what "it shows through
	// what is in front of it" means, and the near cube hides the far one
	// entirely, so there is nowhere else for it to be.
	VOE_TEST_CHECK(far_outlined.count > 100);
	VOE_TEST_CHECK(far_outlined.inside_the_near_cube);

	// And the near cube outlined instead puts the line round that cube's own
	// silhouette, which is the larger of the two: the leftmost outlined
	// pixel of the two frames is not the same pixel.
	VOE_TEST_CHECK(near_outlined.count > 100);
	VOE_TEST_CHECK(near_outlined.leftmost != far_outlined.leftmost);

	voe_render_device_destroy(device);
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
	the_outline_is_drawn_through_what_hides_it(arena, &geometries);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
