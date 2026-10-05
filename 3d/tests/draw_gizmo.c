// The gizmo is drawn through what it stands in (ADR-0205), read off the picture
// through render's public voe_render_target_read.
//
// A dark cube with a gizmo standing in it, drawn twice — once with the gizmo and
// once with the field zeroed. A pixel the arrow along +X covers, inside the
// rectangle the cube covers, is the gizmo's colour in the first picture and the
// cube's in the second. That is the second depth clear doing its work: without
// it the cube the gizmo stands inside is in front of the arrow and the pixel
// stays the cube's. The colour is linear (1, 0, 1), which an sRGB target hands
// back as exactly (255, 0, 255) — nought and one are the two values the curve
// leaves alone — and no lit pixel of a dark cube can be that. The rings (0274)
// are read the same way, at a pixel of the ring about Z inside a cube that fills
// the view.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITHOUT ONE, for the reason
// 3d/tests/import.c gives at length: a box with no Vulkan is the box and not
// this engine.
#include <3d/draw_system.h>
#include <3d/gizmo.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

// The gizmo case's own picture and the room two of them need beside a device:
// wide enough that a shaft three pixels across and the cube it stands in are
// both several pixels of it.
#define GIZMO_SCRATCH (4 * 1024 * 1024)
#define GIZMO_SIDE 128
// One shaft, in pixels: two thirds of the way from the middle to the edge, so
// the near end of the arrow is well inside the cube and its far end well
// outside.
#define GIZMO_PIXELS 60.0f

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
		// Only for the shaped case: the shape system queues removals
		// through it (3d/shape_system.h).
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	voe_3d_panel_register(world, 16);
	return world;
}

// At the origin, looking along its own -Z, so a thing at a more negative z is
// further away.
static void add_a_camera(voe_ecs_world *world)
{
	voe_ecs_entity eye = { 0 };
	voe_scene_transform pose = {
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_transform_add(world, eye, pose));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, lens));
}

// A transform `back` metres down the camera's line of sight, unrotated and
// unscaled.
static voe_scene_transform at_depth(float back)
{
	voe_scene_transform transform = {
		.position = { 0.0f, 0.0f, -back },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	return transform;
}

// One frame of the world with `gizmo` standing in it, and the picture that came
// out. The answer comes back with no gizmo in it and standing one on something
// is a write to that answer, which is the whole of the caller's side of this.
static voe_render_picture a_gizmo_frame(voe_ecs_world *world,
					voe_render_device *device,
					voe_base_arena *arena,
					voe_3d_gizmoed gizmo)
{
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK_INT(frame.gizmo.entity.index, 0);
	VOE_TEST_CHECK_INT(frame.gizmo.entity.generation, 0);
	frame.gizmo = gizmo;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light };

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// WHICH PIXELS OF THE ARROW ALONG +X ARE READ. The arrow runs right from the
// middle of the picture, where the cube's centre is, and the shaft is three
// pixels across — so it is one row off the middle for the whole of its length.
// The cube covers twenty-three pixels either way of the middle: half a metre
// three metres in front of an eye whose vertical field of view is sixty
// degrees, over a picture GIZMO_SIDE tall. The stretch read is well inside
// that and starts well outside the label at the tip of the arrow along +Z,
// which points at the eye and is therefore drawn large across the middle.
#define ARROW_ROW (GIZMO_SIDE / 2 - 1)
#define ARROW_NEAR (GIZMO_SIDE / 2 + 12)
#define ARROW_FAR (GIZMO_SIDE / 2 + 20)

static const uint8_t *pixel_at(voe_render_picture picture, uint32_t x,
			       uint32_t y)
{
	VOE_TEST_CHECK(x < picture.width);
	VOE_TEST_CHECK(y < picture.height);
	return picture.pixels + ((size_t)y * picture.width + x) * 4;
}

// Linear (1, 0, 1), which an sRGB target hands back as exactly (255, 0, 255):
// nought and one are the two values the curve leaves alone.
static bool is_the_gizmos_colour(const uint8_t *pixel)
{
	return pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255;
}

// Whether the whole of that stretch is the gizmo's colour.
static bool the_arrow_covers_the_stretch(voe_render_picture picture)
{
	for (uint32_t x = ARROW_NEAR; x <= ARROW_FAR; x++) {
		if (!is_the_gizmos_colour(pixel_at(picture, x, ARROW_ROW)))
			return false;
	}
	return true;
}

// WHICH PIXEL OF THE RING ABOUT Z IS READ. That ring faces the eye at the
// gizmo's depth, one shaft — GIZMO_PIXELS — about the middle, so the pixel
// (43.5, 41.5) from the middle to its centre is on it: mid-segment, clear of
// the joint at 45 degrees where two segments' quads leave a pixel uncovered.
// The other two rings are seen edge on along the middle row and column.
#define RING_X (GIZMO_SIDE / 2 + 43)
#define RING_Y (GIZMO_SIDE / 2 - 42)

// The rings (0274) show through too: `gizmo` with `rings` set, standing in a
// cube scaled to fill the picture whose near face is a metre and a half in
// front of the ring about Z. A pixel of that ring is the gizmo's colour, and
// without the gizmo it is the cube's — neither the gizmo's nor `background`.
static void the_rings_show_through(voe_ecs_world *world,
				   voe_render_device *device,
				   voe_base_arena *arena, voe_3d_shapes *shapes,
				   voe_3d_gizmoed gizmo,
				   const uint8_t *background)
{
	voe_scene_transform filling = at_depth(3.0f);
	voe_ecs_entity big = { 0 };
	const uint8_t *cube_pixel;

	filling.scale = (voe_math_float3){ 3.0f, 3.0f, 3.0f };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &big));
	VOE_TEST_CHECK(voe_scene_transform_add(world, big, filling));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, big,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.02f, 0.02f, 0.15f } }));
	voe_3d_shape_system_run(world, shapes);

	gizmo.rings = true;
	VOE_TEST_CHECK(is_the_gizmos_colour(pixel_at(
		a_gizmo_frame(world, device, arena, gizmo), RING_X, RING_Y)));
	cube_pixel = pixel_at(a_gizmo_frame(world, device, arena,
					    (voe_3d_gizmoed){ 0 }),
			      RING_X, RING_Y);
	VOE_TEST_CHECK(!is_the_gizmos_colour(cube_pixel));
	VOE_TEST_CHECK(cube_pixel[0] != background[0] ||
		       cube_pixel[1] != background[1] ||
		       cube_pixel[2] != background[2]);
}

// A gizmo stands in the middle of the cube it moves, and it is drawn after
// everything else behind a clear of its own — so a pixel of an arrow that is
// inside the cube is the gizmo's colour and not the cube's (ADR-0205).
static void a_gizmo_shows_through_what_it_stands_in(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' own pools, one object for each of the two cubes and one
	// for each of the gizmo's two draws, and the larger of the two gizmos'
	// geometry — what a program that draws one pays for (3d/draw_system.h).
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 4,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices =
			VOE_3D_GIZMO_VERTICES > VOE_3D_GIZMO_RING_VERTICES ?
				VOE_3D_GIZMO_VERTICES :
				VOE_3D_GIZMO_RING_VERTICES,
		.transient_indices =
			VOE_3D_GIZMO_INDICES > VOE_3D_GIZMO_RING_INDICES ?
				VOE_3D_GIZMO_INDICES :
				VOE_3D_GIZMO_RING_INDICES,
		.transient_geometries = 2,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_3d_gizmoed gizmo;
	voe_render_picture with_a_gizmo;
	voe_render_picture without_one;
	const uint8_t *cube_pixel;
	const uint8_t *background;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	// Lit from above and in front, so the face of the cube turned towards
	// the camera has a colour of its own to be told from the background by.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, sun,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -0.6f, -0.8f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at_depth(3.0f)));
	// Dark, so that no pixel of it can be mistaken for the gizmo's colour
	// however the highlight falls.
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.02f, 0.02f, 0.15f } }));
	voe_3d_shape_system_run(world, &shapes);

	gizmo = (voe_3d_gizmoed){
		.entity = cube,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.marked_colour = { 0.0f, 1.0f, 0.0f },
		.marked = VOE_3D_GIZMO_NONE,
		.pixels = GIZMO_PIXELS,
		.size = { GIZMO_SIDE, GIZMO_SIDE },
	};

	// The gizmo stands inside the cube, so every pixel of that stretch
	// would be the cube's without the second depth clear — the arrow is at
	// the cube's own centre depth and the cube's near face is half a metre
	// in front of it.
	with_a_gizmo = a_gizmo_frame(world, device, arena, gizmo);
	VOE_TEST_CHECK(the_arrow_covers_the_stretch(with_a_gizmo));

	// And the same frame with nothing standing on the cube: the middle of
	// that stretch is the cube's colour, which is neither the gizmo's nor
	// the background's — which is what says the arrow above was drawn
	// through a cube that really covers it.
	without_one = a_gizmo_frame(world, device, arena,
				    (voe_3d_gizmoed){ 0 });
	cube_pixel = pixel_at(without_one, (ARROW_NEAR + ARROW_FAR) / 2,
			      ARROW_ROW);
	background = pixel_at(without_one, 0, 0);
	VOE_TEST_CHECK(!is_the_gizmos_colour(cube_pixel));
	VOE_TEST_CHECK(cube_pixel[0] != background[0] ||
		       cube_pixel[1] != background[1] ||
		       cube_pixel[2] != background[2]);

	the_rings_show_through(world, device, arena, &shapes, gizmo,
			       background);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	a_gizmo_shows_through_what_it_stands_in();
	return voe_test_result();
}
