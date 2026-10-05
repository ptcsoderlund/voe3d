// Every casting directional light casts its own cascades (ADR-0357 point 3),
// through voe_3d_draw_system_shadows between the frame's begin and the view's
// pass.
//
// ONE PICTURE, THREE FLOOR PIXELS, built as 3d/tests/shadows.c builds its. A
// camera at the origin looks along -Z; a flattened cube lies a metre below as
// the floor, and a cube stands a metre above the floor's middle five metres
// out. Two lights in table order, bounces 0, no fill: a sun straight down and a
// moon shining down and toward +x at 45°. The sun's shadow lies under the cube,
// (0, -0.95, -5); the moon's 1.95 m to +x of it, (1.95, -0.95, -5), spanning
// x 0.95 to 2.95. The pixels read are those two and the floor at x = -2, lit by
// both. At five metres the picture spans 2.89 m either way of the middle, so
// they are columns 32, 54 and 10 of row 42, as shadows.c works out.
//
// TWO FRAMES, BECAUSE THE ARRAY GROWS ON THE NEXT. A device starts with one
// light's maps, so the first frame slots the sun alone and asks for two; the
// second slots both, the moon's record reading slot 1. Then both shadows show,
// each darker than the lit floor. The device's capacities fit two lights: 8
// cascade passes and the view, the two casters drawn into each cascade.
//
// THE SUN NOT CASTING LEAVES THE MOON'S: its record is zeroed, the moon takes
// slot 0, the floor under the cube reads as the lit floor and the moon's
// shadow stays.
//
// ONE CASTING LIGHT FITS ONE LIGHT'S PASSES: only the sun casts, on a device
// whose `passes` are its four cascades and the view, as 3d/tests/bounce.c
// budgets. Two frames are true and the array still holds one light.
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

#include <stdio.h>
#include <stdlib.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64
// The three floor pixels, worked out in the header.
#define SUN_X 32
#define MOON_X 54
#define LIT_X 10
#define FLOOR_Y 42
// How far apart, in 8-bit levels, two pixels of the same lit floor may read.
#define LIT_ALIKE 4

// The view's two draws and each caster drawn into each cascade of two lights.
static voe_render_capacities capacities(uint32_t passes)
{
	return (voe_render_capacities){
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2 + 2 * VOE_RENDER_SHADOW_CASCADES * 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = passes,
		.targets = 1,
		.shadow_size = VOE_3D_SHADOW_TEXELS,
	};
}

// One grey cube at (0, `y`, -5), scaled by `scale`, casting.
static void add_a_shape(voe_ecs_world *world, double y, voe_math_float3 scale)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, y, -5.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = scale }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = true }));
}

// A white light shining along `direction`, casting when `casts`.
static void add_a_light(voe_ecs_world *world, voe_math_float3 direction,
			bool casts)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(direction),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.5f,
				   .cast_shadows = casts }));
}

// Who casts: the sun, then the moon, in table order.
typedef struct {
	bool sun;
	bool moon;
} casting;

// The camera, the floor, the cube, the sun and, when `moon`, the moon.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      casting casts, bool moon)
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
	voe_scene_light_register(world, 2);
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
	add_a_light(world, (voe_math_float3){ 0.0f, -1.0f, 0.0f }, casts.sun);
	if (moon)
		add_a_light(world,
			    (voe_math_float3){ 0.70710678f, -0.70710678f, 0.0f },
			    casts.moon);
	add_a_shape(world, -1.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	add_a_shape(world, 1.0, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// The floor of the last of two frames, and its shadow records.
typedef struct {
	uint8_t sun;
	uint8_t moon;
	uint8_t lit;
	voe_render_shadow first;
	voe_render_shadow second;
} floor_pixels;

// Two frames of `world`, each shadows call true; the second frame's pixels
// and records.
static floor_pixels two_frames(voe_ecs_world *world, voe_render_device *device,
			       voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	floor_pixels pixels = { 0 };

	for (uint32_t step = 0; step < 2; step++) {
		voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
		voe_render_pass_camera camera;
		voe_render_picture picture = { 0 };
		voe_base_error error = VOE_BASE_OK;
		bool drawing = false;

		VOE_TEST_CHECK(voe_3d_draw_system_lights(world, &frame, arena));
		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (!drawing)
			return pixels;
		VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
		camera = voe_3d_draw_system_camera(&frame);
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK(voe_render_target_read(device,
						      VOE_RENDER_TARGET_WINDOW,
						      arena, &picture, &error));
		VOE_TEST_CHECK(picture.pixels != NULL);
		if (picture.pixels == NULL)
			return pixels;
		pixels.sun = picture.pixels[((size_t)FLOOR_Y * SIDE + SUN_X) * 4];
		pixels.moon = picture.pixels[((size_t)FLOOR_Y * SIDE + MOON_X) * 4];
		pixels.lit = picture.pixels[((size_t)FLOOR_Y * SIDE + LIT_X) * 4];
		pixels.first = frame.shadow;
		pixels.second = frame.more_count == 1 ?
					frame.more_lights[0].shadow :
					(voe_render_shadow){ 0 };
		printf("frame %u: under the cube %u, to +x %u, lit %u\n", step,
		       pixels.sun, pixels.moon, pixels.lit);
	}
	return pixels;
}

// A sun and a moon both casting: two shadows from the second frame, the moon's
// in slot 1.
static void two_lights_cast_two_shadows(voe_render_device *device,
					const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(
		arena, shapes, (casting){ .sun = true, .moon = true }, true);
	floor_pixels pixels = two_frames(world, device, arena);

	VOE_TEST_CHECK_INT(pixels.first.count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(pixels.first.slot, 0);
	VOE_TEST_CHECK_INT(pixels.second.count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(pixels.second.slot, 1);
	VOE_TEST_CHECK(pixels.sun + 16 < pixels.lit);
	VOE_TEST_CHECK(pixels.moon + 16 < pixels.lit);
	voe_base_arena_destroy(arena);
}

// The sun not casting: its record zeroed, the moon's in slot 0, only the
// moon's shadow on the floor.
static void a_sun_that_does_not_cast_leaves_the_moons(
	voe_render_device *device, const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(
		arena, shapes, (casting){ .sun = false, .moon = true }, true);
	floor_pixels pixels = two_frames(world, device, arena);

	VOE_TEST_CHECK_INT(pixels.first.count, 0);
	VOE_TEST_CHECK_INT(pixels.second.count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(pixels.second.slot, 0);
	VOE_TEST_CHECK(abs((int)pixels.sun - (int)pixels.lit) <= LIT_ALIKE);
	VOE_TEST_CHECK(pixels.moon + 16 < pixels.lit);
	voe_base_arena_destroy(arena);
}

// Only the sun casting, on a device of one light's passes: two frames true and
// the array still one light's.
static void one_casting_light_fits_one_lights_passes(
	voe_render_device *device, const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(
		arena, shapes, (casting){ .sun = true, .moon = false }, true);
	floor_pixels pixels = two_frames(world, device, arena);

	VOE_TEST_CHECK_INT(pixels.first.count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(pixels.second.count, 0);
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(device, 0), 1);
	voe_base_arena_destroy(arena);
}

// A device of `passes` and the shapes on it into `shapes`; NULL, said, with no
// graphics card.
static voe_render_device *a_device(voe_base_arena *arena, uint32_t passes,
				   voe_3d_shapes *shapes)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, size, capacities(passes), &error);

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		return NULL;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, shapes, &error));
	return device;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shapes shapes;
	voe_render_device *device =
		a_device(arena, 2 * VOE_RENDER_SHADOW_CASCADES + 1, &shapes);

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	two_lights_cast_two_shadows(device, &shapes);
	a_sun_that_does_not_cast_leaves_the_moons(device, &shapes);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);

	arena = voe_base_arena_new(SCRATCH);
	device = a_device(arena, VOE_RENDER_SHADOW_CASCADES + 1, &shapes);
	if (device != NULL) {
		one_casting_light_fits_one_lights_passes(device, &shapes);
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
