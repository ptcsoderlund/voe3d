// A cascade draws only the casters inside it (ADR-0464 point 3), and its sun
// side is open, so what casts into the view still does: a headless device as
// 3d/tests/shadows.c builds one, the floor a metre below a camera at the
// origin looking down −Z, and a low sun shining down and towards −Z, two
// along −Z for one down, with no fill, so a shadowed floor reads dark.
//
// 1. A TALL BOX 3 M BEHIND THE CAMERA, a metre square and six tall, throws its
//    shadow twelve metres forward along −Z: the floor under the middle of the
//    picture, (0, −0.95, −5), reads darker than in the same frame without the
//    box. Behind the camera, it is towards the sun from every cascade.
// 2. THE SAME BOX 3 M TO THE SIDE, at x = 3 a metre ahead, is outside the
//    picture, where the half width is under a metre; its shadow runs along
//    x = 3 into it, and the floor there, (2.96, −0.95, −8.1), reads darker
//    than without it.
// 3. THE SAME BOX 2 KM TO THE SIDE is outside every cascade's sides:
//    `frame.counts.cast_skipped` is above the frame's without it, and the
//    picture is that frame's, byte for byte.
//
// WHERE THE PIXELS ARE. A sixty-degree field over 64 square pixels spans
// z tan 30° either way of the middle at depth z: (0, −0.95, −5) is column 32,
// row 42, as in shadows.c; (2.96, −0.95, −8.1) is column 52, row 38, inside a
// shadow seven pixels wide there.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/shadows.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64

// The floor and the box, each drawn into the view and each of the four
// cascades, with one share over; the four shadow passes and the view's.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = (VOE_RENDER_SHADOW_CASCADES + 2) * 2,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = 2 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// One grey cube at (`x`, `y`, `z`), scaled by `scale`, casting.
static void add_a_shape(voe_ecs_world *world, double x, double y, double z,
			voe_math_float3 scale)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { x, y, z },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = scale }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = true }));
}

// The camera, the low sun and the floor, and with `box` a box a metre square
// and six tall standing on the floor at (`x`, `z`).
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      bool box, double x, double z)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing((voe_math_float3){
				0.0f, -0.4472136f, -0.8944272f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .bounces = 0,
				   .cast_shadows = true }));
	add_a_shape(world, 0.0, -1.0, -5.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	if (box)
		add_a_shape(world, x, 2.0, z, (voe_math_float3){ 1.0f, 6.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// One frame of `world` through the shadows call and _run: its picture, on
// `arena`, NULL when none was read, and the frame's counts into `counts`.
static const uint8_t *a_frame(voe_ecs_world *world, voe_render_device *device,
			      voe_base_arena *arena, voe_3d_view_counts *counts)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return NULL;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	VOE_TEST_CHECK_INT(frame.shadow.count, VOE_RENDER_SHADOW_CASCADES);
	*counts = frame.counts;
	camera = voe_3d_draw_system_camera(&frame);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	(void)voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture.pixels;
}

// The red byte at column `x`, row `y`, nought for no picture.
static uint8_t pixel(const uint8_t *picture, uint32_t x, uint32_t y)
{
	return picture != NULL ? picture[((size_t)y * SIDE + x) * 4] : 0;
}

// The floor at column `x`, row `y` darker with a box at (`box_x`, `box_z`)
// than without it.
static void a_box_out_of_view_shadows_it(voe_render_device *device,
					 const voe_3d_shapes *shapes,
					 double box_x, double box_z, uint32_t x,
					 uint32_t y)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_view_counts counts = { 0 };
	const uint8_t *without = a_frame(a_world(arena, shapes, false, 0.0, 0.0),
					 device, arena, &counts);
	uint8_t open = pixel(without, x, y);
	const uint8_t *with = a_frame(a_world(arena, shapes, true, box_x, box_z),
				      device, arena, &counts);
	uint8_t shaded = pixel(with, x, y);

	printf("box at (%.0f, %.0f): floor %u without, %u with\n", box_x, box_z,
	       open, shaded);
	VOE_TEST_CHECK(open > 32);
	VOE_TEST_CHECK(shaded + 32 < open);
	voe_base_arena_destroy(arena);
}

// A box 2 km aside is skipped by the cascades and changes no pixel.
static void a_far_box_is_skipped(voe_render_device *device,
				 const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_view_counts bare = { 0 };
	voe_3d_view_counts boxed = { 0 };
	const uint8_t *without = a_frame(a_world(arena, shapes, false, 0.0, 0.0),
					 device, arena, &bare);
	const uint8_t *with = a_frame(a_world(arena, shapes, true, 2000.0, -5.0),
				      device, arena, &boxed);

	printf("cast skipped %u without, %u with\n", bare.cast_skipped,
	       boxed.cast_skipped);
	VOE_TEST_CHECK(boxed.cast_skipped > bare.cast_skipped);
	VOE_TEST_CHECK_INT(boxed.cast, bare.cast);
	VOE_TEST_CHECK(without != NULL && with != NULL &&
		       memcmp(without, with, (size_t)SIDE * SIDE * 4) == 0);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_shapes shapes;

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

	a_box_out_of_view_shadows_it(device, &shapes, 0.0, 3.0, 32, 42);
	a_box_out_of_view_shadows_it(device, &shapes, 3.0, -1.0, 52, 38);
	a_far_box_is_skipped(device, &shapes);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
