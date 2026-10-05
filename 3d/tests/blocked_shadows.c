// A light a blocker holds is shadowed only by the casters those blockers hold
// (ADR-0361 point 2), through voe_3d_draw_system_shadows.
//
// ONE PICTURE, TWO FLOOR PIXELS, built as 3d/tests/shadow_lights.c builds its,
// device and all. A camera at the origin looks along -Z; a flattened cube lies a
// metre below as the floor, and a cube stands a metre above the floor's middle
// five metres out. A blocker box 6 × 4 × 4 about (0, 0.5, -5) holds both their
// origins and the floor the pixels read; a second flattened cube at y 3, its
// origin outside the box, is a roof over it all. A sun of intensity 0, then a
// moon at (0, 2, -5) inside the box shining straight down, both bounces 0 and no
// fill. The pixels are row 42's columns 10, the floor in the open, and 32, the
// floor under the cube, as shadow_lights.c works them out.
//
// EACH CASE READS THE SECOND FRAME, as shadow_lights.c does, its blockers
// filled before its lights so each light's mask is taken.
//
// 1. FOR EACH BLOCK KIND, ALL, DIRECT AND FILL: the moon casting gives the open
//    pixel within 2 of it not casting, per channel, and well above black, since
//    the roof outside the box casts nothing for it; under the cube is darker.
// 2. NO BLOCKER, THE MOON CASTING: the roof shadows the floor, the open pixel
//    near black.
// 3. THE MOON AS THE ONLY LIGHT, LIGHT 0, inside an All box and casting: as 1,
//    its mask the blockers' `sun`.
//
// The device's capacities fit one casting light: its four cascade passes and
// the view, the three casters drawn into every pass.
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
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>
#include <stdlib.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64
// The two floor pixels, worked out in the header.
#define UNDER_X 32
#define OPEN_X 10
#define FLOOR_Y 42
// No blocker in the world.
#define NO_BLOCKER 99u
// How far, in 8-bit levels, the open floor may move when the moon casts.
#define ALIKE 2
// Lit is above this; near black below NEAR_BLACK.
#define WELL_LIT 32
#define NEAR_BLACK 16

// Three casters drawn into every pass, as shadow_lights.c sizes them.
static voe_render_capacities capacities(uint32_t passes)
{
	return (voe_render_capacities){
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 3 * passes,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = passes,
		.targets = 1,
		.shadow_size = VOE_3D_SHADOW_TEXELS,
		.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
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

// A white light row of `intensity`, bounces 0, casting when `casts`.
static voe_scene_light a_light(float intensity, bool casts)
{
	return (voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				  .intensity = intensity,
				  .bounce_strength = 1.0f,
				  .cast_shadows = casts };
}

// The light `row` at `at` shining straight down.
static void add_a_light(voe_ecs_world *world, voe_math_double3 at,
			voe_scene_light row)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = at,
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, row));
}

// The box of Block kind `block` about the floor, the cube and the moon.
static void add_a_blocker(voe_ecs_world *world, uint32_t block)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, 0.5, -5.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, entity,
		(voe_scene_light_blocker){ .size = { 6.0f, 4.0f, 4.0f },
					   .block = block }));
}

// The camera, the sun when `with_sun`, the `moon`, the floor, the cube, the
// roof, and a blocker of `block` unless NO_BLOCKER.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      bool with_sun, voe_scene_light moon,
			      uint32_t block)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 12,
		.intent_types = 12,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 2);
	voe_scene_light_blocker_register(world, 1);
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
	if (with_sun)
		add_a_light(world, (voe_math_double3){ 0.0, 2.0, -5.0 },
			    a_light(0.0f, true));
	add_a_light(world, (voe_math_double3){ 0.0, 2.0, -5.0 }, moon);
	add_a_shape(world, -1.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	add_a_shape(world, 1.0, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	add_a_shape(world, 3.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	if (block != NO_BLOCKER)
		add_a_blocker(world, block);
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// The two floor pixels of a frame, red, green and blue.
typedef struct {
	uint8_t open[3];
	uint8_t under[3];
} floor_pixels;

// Two frames of `world`, each shadows call true; the second's pixels.
static floor_pixels frames_of(voe_ecs_world *world, voe_render_device *device,
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

		VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame,
								 arena));
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
		for (uint32_t c = 0; c < 3; c++) {
			pixels.open[c] = picture.pixels
				[((size_t)FLOOR_Y * SIDE + OPEN_X) * 4 + c];
			pixels.under[c] = picture.pixels
				[((size_t)FLOOR_Y * SIDE + UNDER_X) * 4 + c];
		}
	}
	printf("open %u %u %u, under the cube %u %u %u\n", pixels.open[0],
	       pixels.open[1], pixels.open[2], pixels.under[0], pixels.under[1],
	       pixels.under[2]);
	return pixels;
}

// The second frame's pixels of a fresh world.
static floor_pixels drawn(voe_render_device *device,
			  const voe_3d_shapes *shapes, bool with_sun,
			  bool casts, uint32_t block)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(arena, shapes, with_sun,
				       a_light(1.5f, casts), block);
	floor_pixels pixels = frames_of(world, device, arena);

	voe_base_arena_destroy(arena);
	return pixels;
}

// Cases 1 and 3: the open floor as with no shadows, lit, and the cube's
// shadow darker.
static void shadowed_only_inside(voe_render_device *device,
				 const voe_3d_shapes *shapes, bool with_sun,
				 uint32_t block)
{
	floor_pixels still = drawn(device, shapes, with_sun, false, block);
	floor_pixels cast = drawn(device, shapes, with_sun, true, block);

	for (uint32_t c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)cast.open[c] - (int)still.open[c]) <=
			       ALIKE);
	VOE_TEST_CHECK(cast.open[0] > WELL_LIT);
	VOE_TEST_CHECK(cast.under[0] < cast.open[0]);
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
	const uint32_t kinds[3] = { VOE_SCENE_LIGHT_BLOCKER_ALL,
				    VOE_SCENE_LIGHT_BLOCKER_DIRECT,
				    VOE_SCENE_LIGHT_BLOCKER_FILL };
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shapes shapes;
	voe_render_device *device =
		a_device(arena, VOE_RENDER_SHADOW_CASCADES + 1, &shapes);

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	for (uint32_t kind = 0; kind < 3; kind++) {
		printf("block %u:\n", kinds[kind]);
		shadowed_only_inside(device, &shapes, true, kinds[kind]);
	}
	printf("no blocker:\n");
	VOE_TEST_CHECK(drawn(device, &shapes, true, true, NO_BLOCKER).open[0] <
		       NEAR_BLACK);
	printf("the moon alone:\n");
	shadowed_only_inside(device, &shapes, false,
			     VOE_SCENE_LIGHT_BLOCKER_ALL);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
