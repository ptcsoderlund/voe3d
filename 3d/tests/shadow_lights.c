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
// cascade passes and the view, the casters drawn into every pass.
//
// THE SUN NOT CASTING LEAVES THE MOON'S: its record is zeroed, the moon takes
// slot 0, the floor under the cube reads as the lit floor and the moon's
// shadow stays.
//
// ONE CASTING LIGHT FITS ONE LIGHT'S PASSES: only the sun casts, on a device
// whose `passes` are its four cascades and the view, as 3d/tests/bounce.c
// budgets. Two frames are true and the array still holds one light.
//
// A MOON BOUNCES (0357 point 1): a sun of intensity 0 and bounces 0 and a moon
// of intensity 3 and bounces 1, and a wall 0.2 × 8 × 4 at x 4 up to y 7 whose
// -x face the moon lights, since the floor in the cube's shadow otherwise sees
// only the cube's dark side and the black sky above the probes. That floor is
// read on frame 3 + FADE: frame two captures the probes nearest the eye, and a
// new picture fades in over FADE places (0389 point 4). It is brighter than
// with the moon's bounces at 0.
//
// EACH CASTING SUN THAT BOUNCES DRAWS ITS OWN MAP (0357 point 4, 0389): the
// world fits a 2 m level grid, so the 1 m nest begins beside it, as
// volumes_begun works out from bounce_grid.h. The sun and the moon both
// bouncing and casting, the second frame's shadows call, no view pass, as
// bounce.c counts, is false on a device whose passes are one short of both
// lights' cascades, the capture passes and a bounce shadow pass per casting sun
// per begun volume, and true with one more. Every device has objects for the
// most passes a frame here opens. Both bounce cases skip, said, without
// shaderOutputLayer.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/shadows.c does.
#include <3d/bounce_grid.h>
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
// Where the wall the moon lights stands, past its shadow and behind the floor
// the pixels read.
#define WALL_X 4.0
// How far apart, in 8-bit levels, two pixels of the same lit floor may read.
#define LIT_ALIKE 4

// Places a new probe picture takes to full weight (0389 point 4), render's
// VOE_RENDER_BOUNCE_FADE, which render keeps to itself.
#define FADE 16
// The most passes a frame here opens: both lights' cascades and a sun map per
// volume a target holds, the capture passes and the view.
#define MOST_PASSES                                                       \
	(2 * (VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_VOLUMES) +   \
	 VOE_RENDER_BOUNCE_CAPTURE_PASSES + 1)

// Up to three casters, the wall's included, drawn into every pass a frame can
// open, as bounce.c sizes them; point shadows sized so their readiness says
// whether the card has shaderOutputLayer, as bounce.c does.
static voe_render_capacities capacities(uint32_t passes)
{
	return (voe_render_capacities){
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 3 * MOST_PASSES,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = passes,
		.targets = 1,
		.shadow_size = VOE_3D_SHADOW_TEXELS,
		.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
	};
}

// The volumes the world begins, with the wall when `wall`: the level grid
// fitted to the still casters' box, the floor, the cube and the wall worked
// out by hand about the eye at the origin, and each nest finer than it.
static uint32_t volumes_begun(bool wall)
{
	voe_math_double3 min = { -10.0, -1.05, -15.0 };
	voe_math_double3 max = { 10.0, wall ? 7.0 : 1.5, 5.0 };
	float level = voe_3d_bounce_grid_fit(min, max, (voe_math_double3){ 0 })
			      .spacing;
	uint32_t begun = 1;

	for (uint32_t nest = 0; nest < VOE_3D_BOUNCE_NESTS; nest++)
		if (voe_3d_bounce_nest_spacing(nest) < level)
			begun++;
	return begun;
}

// The passes of the bounce's frame for `lights` casting and bouncing in a
// world of `begun` volumes: their cascades, the capture passes and a bounce
// shadow pass each per volume.
static uint32_t bounce_passes(uint32_t lights, uint32_t begun)
{
	return lights * (VOE_RENDER_SHADOW_CASCADES + begun) +
	       VOE_RENDER_BOUNCE_CAPTURE_PASSES;
}

// One grey cube at (`x`, `y`, -5), scaled by `scale`, casting.
static void add_a_shape(voe_ecs_world *world, double x, double y,
			voe_math_float3 scale)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { x, y, -5.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = scale }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = true }));
}

// A white light row of `intensity` and `bounces`, at strength 1, casting when
// `casts`.
static voe_scene_light a_light(float intensity, uint32_t bounces, bool casts)
{
	return (voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				  .intensity = intensity,
				  .bounces = bounces,
				  .bounce_strength = 1.0f,
				  .cast_shadows = casts };
}

// The light `row` shining along `direction`.
static void add_a_light(voe_ecs_world *world, voe_math_float3 direction,
			voe_scene_light row)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(direction),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, row));
}

// The camera, the floor, the cube, then the `sun` and the `moon` rows.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      voe_scene_light sun, voe_scene_light moon)
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
	add_a_light(world, (voe_math_float3){ 0.0f, -1.0f, 0.0f }, sun);
	add_a_light(world, (voe_math_float3){ 0.70710678f, -0.70710678f, 0.0f },
		    moon);
	add_a_shape(world, 0.0, -1.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f });
	add_a_shape(world, 0.0, 1.0, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// The floor of the last frame, and its shadow records.
typedef struct {
	uint8_t sun;
	uint8_t moon;
	uint8_t lit;
	voe_render_shadow first;
	voe_render_shadow second;
} floor_pixels;

// `frames` frames of `world`, each shadows call true; the last frame's pixels
// and records.
static floor_pixels frames_of(voe_ecs_world *world, voe_render_device *device,
			      voe_base_arena *arena, uint32_t frames)
{
	voe_platform_size size = { SIDE, SIDE };
	floor_pixels pixels = { 0 };

	for (uint32_t step = 0; step < frames; step++) {
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
	voe_ecs_world *world = a_world(arena, shapes, a_light(1.5f, 0, true),
				       a_light(1.5f, 0, true));
	floor_pixels pixels = frames_of(world, device, arena, 2);

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
	voe_ecs_world *world = a_world(arena, shapes, a_light(1.5f, 0, false),
				       a_light(1.5f, 0, true));
	floor_pixels pixels = frames_of(world, device, arena, 2);

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
	voe_ecs_world *world = a_world(arena, shapes, a_light(1.5f, 0, true),
				       a_light(1.5f, 0, false));
	floor_pixels pixels = frames_of(world, device, arena, 2);

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

// The floor in the moon's shadow on frame 3 + FADE, on a fresh device, for a
// sun of intensity 0 and bounces 0, a moon of intensity 3 and `bounces`, and
// the wall the moon lights; 0 with no device or no shaderOutputLayer, which
// `skipped` says.
static uint8_t moon_shadow(uint32_t bounces, bool *skipped)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shapes shapes;
	voe_render_device *device = a_device(
		arena, bounce_passes(1, volumes_begun(true)) + 1, &shapes);
	uint8_t pixel = 0;

	*skipped = device == NULL || !voe_render_point_shadows_ready(device);
	if (!*skipped) {
		voe_ecs_world *world = a_world(arena, &shapes,
					       a_light(0.0f, 0, true),
					       a_light(3.0f, bounces, true));

		add_a_shape(world, WALL_X, 3.0,
			    (voe_math_float3){ 0.2f, 8.0f, 4.0f });
		voe_3d_shape_system_run(world, &shapes);
		pixel = frames_of(world, device, arena, 3 + FADE).moon;
	}
	if (device != NULL)
		voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return pixel;
}

// The second frame's shadows call, no view pass, on a fresh device of
// `passes`, for the sun and the moon both bouncing and casting. True with no
// device or no shaderOutputLayer, which `skipped` says.
static bool both_bounce_on(uint32_t passes, bool *skipped)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_shapes shapes;
	voe_render_device *device = a_device(arena, passes, &shapes);
	bool answer = true;

	*skipped = device == NULL || !voe_render_point_shadows_ready(device);
	if (!*skipped) {
		voe_ecs_world *world = a_world(arena, &shapes,
					       a_light(1.5f, 1, true),
					       a_light(1.5f, 1, true));

		for (uint32_t step = 0; step < 2; step++) {
			voe_3d_frame frame =
				voe_3d_draw_system_frame(world, size, 0.0f);
			bool drawing = false;

			VOE_TEST_CHECK(voe_3d_draw_system_lights(world, &frame, arena));
			VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
			VOE_TEST_CHECK(drawing);
			if (!drawing)
				break;
			answer = voe_3d_draw_system_shadows(world, device, &frame);
			VOE_TEST_CHECK(voe_render_frame_end(device));
		}
	}
	if (device != NULL)
		voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return answer;
}

// Cases 4 and 5 of the header, each skipped, said, without shaderOutputLayer.
static void the_suns_bounce(void)
{
	bool skipped = false;
	uint8_t bounced = moon_shadow(1, &skipped);
	uint8_t still = moon_shadow(0, &skipped);

	if (skipped) {
		printf("skip: no shaderOutputLayer, so nothing bounces\n");
		return;
	}
	printf("moon's shadow: bounced %u, not %u\n", bounced, still);
	VOE_TEST_CHECK(bounced > still);
	VOE_TEST_CHECK(!both_bounce_on(bounce_passes(2, volumes_begun(false)) - 1,
				       &skipped));
	VOE_TEST_CHECK(both_bounce_on(bounce_passes(2, volumes_begun(false)),
				      &skipped));
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
	the_suns_bounce();
	return voe_test_result();
}
