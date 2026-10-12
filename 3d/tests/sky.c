// A frame carries the world's sky and picture (0406 points 1 and 4, 0407
// point 1), through voe_3d_draw_system_frame and voe_3d_draw_system_camera.
//
// THE RECORDS, NO GRAPHICS CARD: a world with transforms, a camera and a light
// but no sky table frames a zeroed `sky`; with the table and no row, zeroed
// too; with a row of values none of them the default, `drawn` is 1, each field
// is the row's and `fill` the light's fill intensity, 0 with the light table
// empty. No picture table frames a zeroed `exposure`; a row of exposure -1 and
// Clip is copied, and the pass camera carries both.
//
// AND THE PICTURE: a camera 1 m up looking slightly down at a flat red slab
// under a sun overhead, with the default sky row, drawn through _frame,
// _camera and _run. The top left corner is sky, the zenith's blue above red
// and not the clear colour; the slab's centre reads red and lit, and darker
// under a picture row of exposure -2. Without the sky row the corner is the
// clear colour, taken from a pass that draws nothing. Needs a graphics card
// and skips with a reason without one, as 3d/tests/light_blockers.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
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
#include <scene/picture_component.h>
#include <scene/picture_system.h>
#include <scene/sky_component.h>
#include <scene/sky_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
#define FILL 0.75f

static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
				       .near_plane = 0.1f,
				       .far_plane = 100.0f };

// 0406 point 1's default row.
static const voe_scene_sky DEFAULT_SKY = { .zenith = { 0.15f, 0.35f, 0.80f },
					   .horizon = { 0.60f, 0.75f, 0.90f },
					   .ground = { 0.20f, 0.18f, 0.16f },
					   .horizon_height = 0.25f,
					   .sun_size = 1.5f,
					   .sun_glow = 0.5f,
					   .sunset = 0.5f,
					   .haze = 1000.0f };

// What a world is built with: which tables beyond the drawing ones, and the
// rows in them; NULL adds no row.
struct setup {
	bool sky_table;
	const voe_scene_sky *sky;
	bool picture_table;
	const voe_scene_picture *picture;
	bool no_light;
};

// A camera 1 m up at z = 5 looking at the origin, a sun shining straight down
// with fill intensity FILL unless `no_light`, and the tables and rows `setup`
// asks for.
static voe_ecs_world *a_world(voe_base_arena *arena, struct setup setup)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = 8,
					 .component_types = 16,
					 .intent_types = 16,
					 .structure_requests = 16,
					 .structure_bytes = 1024 });
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 4);
	voe_3d_model_register(world, 1);
	if (setup.sky_table)
		voe_scene_sky_register(world, 1);
	if (setup.picture_table)
		voe_scene_picture_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 1.0, 5.0 },
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, -5.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, LENS));

	if (!setup.no_light) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, entity,
			(voe_scene_transform){
				.rotation = voe_scene_light_facing(
					(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
				.scale = { 1.0f, 1.0f, 1.0f } }));
		VOE_TEST_CHECK(voe_scene_light_add(
			world, entity,
			(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
					   .intensity = 3.0f,
					   .fill_colour = { 1.0f, 1.0f, 1.0f },
					   .fill_intensity = FILL }));
	}

	if (setup.sky != NULL) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_sky_add(world, entity, *setup.sky));
	}
	if (setup.picture != NULL) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_picture_add(world, entity, *setup.picture));
	}
	return world;
}

static void check_float3(voe_math_float3 got, voe_math_float3 want)
{
	VOE_TEST_CHECK_FLOAT(got.x, want.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(got.y, want.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(got.z, want.z, 0.0f);
}

static void check_zeroed(voe_render_sky sky)
{
	VOE_TEST_CHECK_INT(sky.drawn, 0);
	check_float3(sky.zenith, (voe_math_float3){ 0 });
	check_float3(sky.horizon, (voe_math_float3){ 0 });
	check_float3(sky.ground, (voe_math_float3){ 0 });
	VOE_TEST_CHECK_FLOAT(sky.horizon_height, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sun_size, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sun_glow, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.fill, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.haze, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sunset, 0.0f, 0.0f);
}

static void check_row(voe_render_sky sky, voe_scene_sky row, float fill)
{
	VOE_TEST_CHECK_INT(sky.drawn, 1);
	check_float3(sky.zenith, row.zenith);
	check_float3(sky.horizon, row.horizon);
	check_float3(sky.ground, row.ground);
	VOE_TEST_CHECK_FLOAT(sky.horizon_height, row.horizon_height, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sun_size, row.sun_size, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sun_glow, row.sun_glow, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.fill, fill, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.haze, row.haze, 0.0f);
	VOE_TEST_CHECK_FLOAT(sky.sunset, row.sunset, 0.0f);
}

static void the_sky_is_the_first_row(voe_base_arena *arena)
{
	voe_platform_size size = { 640, 480 };
	voe_scene_sky row = { .zenith = { 0.1f, 0.2f, 0.3f },
			      .horizon = { 0.4f, 0.5f, 0.6f },
			      .ground = { 0.7f, 0.8f, 0.9f },
			      .horizon_height = 0.6f,
			      .sun_size = 3.0f,
			      .sun_glow = 0.25f,
			      .sunset = 0.75f,
			      .haze = 500.0f };
	voe_3d_frame frame;
	voe_render_pass_camera camera;

	check_zeroed(voe_3d_draw_system_frame(a_world(arena, (struct setup){ 0 }),
					      size, 0.0f)
			     .sky);
	check_zeroed(voe_3d_draw_system_frame(
			     a_world(arena, (struct setup){ .sky_table = true }),
			     size, 0.0f)
			     .sky);

	frame = voe_3d_draw_system_frame(
		a_world(arena, (struct setup){ .sky_table = true, .sky = &row }),
		size, 0.0f);
	camera = voe_3d_draw_system_camera(&frame);
	check_row(frame.sky, row, FILL);
	check_row(camera.sky, row, FILL);

	check_row(voe_3d_draw_system_sky(a_world(
			  arena, (struct setup){ .sky_table = true,
						 .sky = &row,
						 .no_light = true })),
		  row, 0.0f);
}

static void the_exposure_is_the_first_picture_row(voe_base_arena *arena)
{
	voe_platform_size size = { 640, 480 };
	voe_scene_picture row = { .exposure = -1.0f,
				  .tone = VOE_SCENE_PICTURE_TONE_CLIP };
	voe_3d_frame frame;
	voe_render_pass_camera camera;

	frame = voe_3d_draw_system_frame(a_world(arena, (struct setup){ 0 }), size,
					 0.0f);
	VOE_TEST_CHECK_FLOAT(frame.exposure.exposure, 0.0f, 0.0f);
	VOE_TEST_CHECK_INT(frame.exposure.tone, 0);

	frame = voe_3d_draw_system_frame(
		a_world(arena,
			(struct setup){ .picture_table = true, .picture = &row }),
		size, 0.0f);
	camera = voe_3d_draw_system_camera(&frame);
	for (int i = 0; i < 2; i++) {
		voe_render_exposure exposure = i == 0 ? frame.exposure :
							camera.exposure;

		VOE_TEST_CHECK_FLOAT(exposure.exposure, -1.0f, 0.0f);
		VOE_TEST_CHECK_INT(exposure.tone, VOE_RENDER_TONE_CLIP);
	}
}

static const uint8_t *pixel_at(voe_render_picture picture, uint32_t column,
			       uint32_t row)
{
	const uint8_t *pixel =
		picture.pixels + ((size_t)row * picture.width + column) * 4;

	printf("pixel %u %u: %u %u %u\n", column, row, pixel[0], pixel[1],
	       pixel[2]);
	return pixel;
}

// One frame of `world`, through _frame, _camera and _run when `run`, read
// back; no pixels when it could not be drawn.
static voe_render_picture drawn(voe_ecs_world *world, voe_render_device *device,
				voe_base_arena *arena, bool run)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera = voe_3d_draw_system_camera(&frame);
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	if (!run)
		camera = (voe_render_pass_camera){ .view = frame.view };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	if (run)
		voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// A red slab 4 × 0.1 × 4 m about the origin, its top at y = 0.05.
static void add_slab(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_ecs_entity slab = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &slab));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, slab,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 4.0f, 0.1f, 4.0f } }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, slab,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 1.0f, 0.0f, 0.0f } }));
	voe_3d_shape_system_run(world, shapes);
}

static void the_sky_is_drawn_behind_the_slab(voe_base_arena *arena,
					     voe_render_device *device,
					     const voe_3d_shapes *shapes)
{
	voe_scene_picture darker = { .exposure = -2.0f,
				     .tone = VOE_SCENE_PICTURE_TONE_CLIP };
	voe_ecs_world *skied = a_world(
		arena, (struct setup){ .sky_table = true, .sky = &DEFAULT_SKY });
	voe_ecs_world *exposed = a_world(
		arena, (struct setup){ .sky_table = true,
				       .sky = &DEFAULT_SKY,
				       .picture_table = true,
				       .picture = &darker });
	voe_ecs_world *bare = a_world(arena, (struct setup){ .sky_table = true });
	voe_render_picture clear = drawn(bare, device, arena, false);
	voe_render_picture sky;
	voe_render_picture dim;
	voe_render_picture none;

	add_slab(skied, shapes);
	add_slab(exposed, shapes);
	add_slab(bare, shapes);
	sky = drawn(skied, device, arena, true);
	dim = drawn(exposed, device, arena, true);
	none = drawn(bare, device, arena, true);
	if (clear.pixels == NULL || sky.pixels == NULL || dim.pixels == NULL ||
	    none.pixels == NULL)
		return;

	const uint8_t *background = pixel_at(clear, 0, 0);
	const uint8_t *corner = pixel_at(sky, 0, 0);
	const uint8_t *centre = pixel_at(sky, SIDE / 2, SIDE / 2);
	const uint8_t *dimmed = pixel_at(dim, SIDE / 2, SIDE / 2);
	const uint8_t *unskied = pixel_at(none, 0, 0);
	bool differs = false;

	VOE_TEST_CHECK(corner[2] > corner[0]);
	for (int channel = 0; channel < 3; channel++) {
		differs = differs || corner[channel] != background[channel];
		VOE_TEST_CHECK_INT(unskied[channel], background[channel]);
	}
	VOE_TEST_CHECK(differs);
	VOE_TEST_CHECK(centre[0] > 100);
	VOE_TEST_CHECK(centre[0] > centre[1] + 50 && centre[0] > centre[2] + 50);
	VOE_TEST_CHECK(dimmed[0] < centre[0]);
}

// The slab in one object and nothing of this frame's geometry.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = 1,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = 1,
};

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_3d_shapes shapes;

	the_sky_is_the_first_row(arena);
	the_exposure_is_the_first_picture_row(arena);

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	the_sky_is_drawn_behind_the_slab(arena, device, &shapes);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
