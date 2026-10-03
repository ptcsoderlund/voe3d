// The shadows call drives the probe bounce (ADR-0326 point 8): after its
// point-shadow pass it begins the window's bounce, opens capture passes and
// relights, and a caster that moved this step marks the volume stale where it
// was and where it is.
//
// ONE WORLD. A camera at the origin looks along -Z; a grey ground lies a metre
// below, a red wall stands two metres to the right and five out, and a sun
// shines down and to the left onto it. The sun and both shapes say
// `cast_shadows = true`, since a literal's zero casts nothing (0324), but where
// a sun that does not cast is the case. A lamp, when there is one, casts
// nothing, so no point-shadow pass opens.
//
// THE PASS COUNTS, TWO FRAMES ON ONE DEVICE. With the sun at bounces 1, frame
// one only asks for the volume and opens no capture pass: true on the four
// cascades' passes alone. Frame two captures four times and draws the
// relight's sun map (0329): true with exactly four cascades, four capture
// passes and the bounce shadow pass, false with one fewer. At bounces 0 and no
// lamp that bounces nothing is begun: both frames true on four, with
// `frame.shadow` the four cascades. A lamp of bounces 1 under a sun of 0
// bounces as the sun did but opens no sun map, the sun not bouncing. A sun of
// bounces 1 that does not cast opens no cascade and no sun map: frame two true
// on the four capture passes, false on three. A card without shaderOutputLayer,
// read here as point shadows not ready, captures nothing: frame two true on
// the cascades' passes, and said.
//
// TWO VIEWS IN ONE FRAME, as the editor draws them: 051's bug 02, a second
// view's sun map asserting. Each frame calls the shadows call for the window
// and again for a made target, its eye 3 m along X, the sun at bounces 1. Frame
// one is true on both. Frame two opens each view's four cascades and its own
// sun map (0330), and the capture passes the frame's budget of four gives,
// all the first view's: true on exactly those, false on one fewer.
//
// THE STALE SPHERES. With no previous table nothing moved, so none. With one,
// the wall remembered and then moved a metre along X marks two spheres of
// VOE_3D_BOUNCE_REACH, at (2, 0, -5) and (3, 0, -5) about the eye; a room of
// one gives one; remembered again, unmoved, it marks none. The ground never
// moves and marks nothing throughout.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/shadows.c does.
#include "../src/draw_bounce.h"

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
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32

// The two casters drawn into four cascades and four capture passes, with room
// over; point shadows sized so their readiness says whether the card has
// shaderOutputLayer.
static voe_render_capacities capacities(uint32_t passes)
{
	return (voe_render_capacities){
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 32,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = passes,
		.targets = 1,
		.shadow_size = VOE_3D_SHADOW_TEXELS,
		.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
	};
}

// The passes of four cascades, four capture passes and the bounce shadow pass.
#define ALL_PASSES \
	(VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES + 1)

// One coloured cube `at`, scaled by `scale`; the entity.
static voe_ecs_entity add_a_shape(voe_ecs_world *world, voe_math_double3 at,
				  voe_math_float3 scale, voe_math_float3 colour)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = at,
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = scale }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = colour,
				.cast_shadows = true }));
	return entity;
}

// A lamp of `bounces` at (0, 1.5, -3) that casts nothing.
static void add_a_lamp(voe_ecs_world *world, uint32_t bounces)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, 1.5, -3.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, entity,
		(voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					 .intensity = 3.0f,
					 .range = 6.0f,
					 .falloff = 1.0f,
					 .bounces = bounces,
					 .bounce_strength = 1.0f }));
}

// The camera, the sun of `bounces` that casts when `casts`, the ground and the
// wall, whose entity goes to `wall`, and a lamp of `lamp_bounces` when that is
// above nought; with `previous` the world keeps a previous table.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      bool previous, uint32_t bounces, bool casts,
			      uint32_t lamp_bounces, voe_ecs_entity *wall)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	if (previous)
		voe_scene_transform_previous_register(world, 8);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_scene_point_light_register(world, 2);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 8);

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
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ -0.70710678f, -0.70710678f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .bounces = bounces,
				   .bounce_strength = 1.0f,
				   .cast_shadows = casts }));
	(void)add_a_shape(world, (voe_math_double3){ 0.0, -1.0, -5.0 },
			  (voe_math_float3){ 20.0f, 0.1f, 20.0f },
			  (voe_math_float3){ 0.5f, 0.5f, 0.5f });
	*wall = add_a_shape(world, (voe_math_double3){ 2.0, 0.0, -5.0 },
			    (voe_math_float3){ 0.2f, 2.0f, 4.0f },
			    (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	if (lamp_bounces > 0)
		add_a_lamp(world, lamp_bounces);
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// What the shadows call answered in each of two frames on one fresh device.
typedef struct {
	bool first;
	bool second;
	uint32_t cascades;
} two_answers;

// Two frames on a fresh device with `passes`, for a sun of `bounces` that casts
// when `casts` and a lamp of `lamp_bounces`; the cascades are the second
// frame's. With `two_views` each frame calls the shadows call again, for a made
// target seen from 3 m along X, and an answer is both calls'. Both answers true
// when no device could be made, which the caller has already skipped for.
static two_answers two_frames(uint32_t passes, uint32_t bounces, bool casts,
			      uint32_t lamp_bounces, bool two_views)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, size, capacities(passes), &error);
	two_answers answers = { true, true, 0 };
	voe_render_target target = VOE_RENDER_TARGET_WINDOW;
	voe_render_texture picture;
	voe_3d_shapes shapes;
	voe_ecs_entity wall;
	voe_ecs_world *world;

	VOE_TEST_CHECK(device != NULL);
	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return answers;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	if (two_views)
		VOE_TEST_CHECK(voe_render_target_create(device, SIDE, SIDE, &target,
							&picture, &error));
	world = a_world(arena, &shapes, false, bounces, casts, lamp_bounces, &wall);
	for (int step = 0; step < 2; step++) {
		voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
		voe_3d_frame other = frame;
		bool drawing = false;
		bool answer = false;

		other.target = target;
		other.eye.x += 3.0;
		VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
		VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &other, arena));
		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (drawing) {
			answer = voe_3d_draw_system_shadows(world, device, &frame);
			if (two_views &&
			    !voe_3d_draw_system_shadows(world, device, &other))
				answer = false;
			VOE_TEST_CHECK(voe_render_frame_end(device));
		}
		*(step == 0 ? &answers.first : &answers.second) = answer;
		answers.cascades = frame.shadow.count;
	}
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return answers;
}

// A sun of `bounces`, casting when `casts`, and a lamp of `lamp_bounces` that
// bounce between them: frame one true on the cascades' passes alone (one when
// the sun casts none), and frame two, with shaderOutputLayer (`captures`), true
// on the cascades', the capture passes and the sun map when the sun bounces and
// casts, and false on one fewer; without, true on the cascades' passes.
static void it_bounces(uint32_t bounces, bool casts, uint32_t lamp_bounces,
		       bool captures)
{
	uint32_t cascades = casts ? VOE_RENDER_SHADOW_CASCADES : 0;
	uint32_t wanted = cascades + VOE_RENDER_BOUNCE_CAPTURE_PASSES +
			  (casts && bounces >= 1 ? 1 : 0);
	two_answers cascades_only = two_frames(cascades > 0 ? cascades : 1,
					       bounces, casts, lamp_bounces, false);

	VOE_TEST_CHECK(cascades_only.first);
	if (!captures) {
		VOE_TEST_CHECK(cascades_only.second);
		return;
	}
	VOE_TEST_CHECK(
		two_frames(wanted, bounces, casts, lamp_bounces, false).second);
	VOE_TEST_CHECK(
		!two_frames(wanted - 1, bounces, casts, lamp_bounces, false).second);
}

// Two views of a sun at bounces 1 that casts: frame one true, and frame two
// true on both views' cascades and sun maps and the frame's capture passes and
// false on one fewer; without shaderOutputLayer true on the cascades' alone.
static void it_bounces_in_two_views(bool captures)
{
	uint32_t cascades = 2 * VOE_RENDER_SHADOW_CASCADES;
	uint32_t wanted = cascades + VOE_RENDER_BOUNCE_CAPTURE_PASSES + 2;
	two_answers enough =
		two_frames(captures ? wanted : cascades, 1, true, 0, true);

	VOE_TEST_CHECK(enough.first && enough.second);
	if (captures)
		VOE_TEST_CHECK(!two_frames(wanted - 1, 1, true, 0, true).second);
}

// Whether `sphere` is centred at `x`, 0, -5 with the reach for its radius.
static bool centred(voe_math_float4 sphere, float x)
{
	printf("sphere %g %g %g r %g\n", sphere.x, sphere.y, sphere.z, sphere.w);
	return fabsf(sphere.x - x) < 1e-4f && fabsf(sphere.y) < 1e-4f &&
	       fabsf(sphere.z + 5.0f) < 1e-4f && sphere.w == VOE_3D_BOUNCE_REACH;
}

// No previous table marks nothing; with one, a metre's move marks two, a room
// of one one, and an unmoved wall none.
static void moved_casters_mark_spheres(const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, false, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_scene_transform moved;

	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, &frame, spheres, 8), 0);

	world = a_world(arena, shapes, true, 1, true, 0, &wall);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_scene_transform_remember(world);
	moved = *voe_scene_transform_get(world, wall);
	moved.position.x += 1.0;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ wall, moved }));
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, &frame, spheres, 8), 2);
	VOE_TEST_CHECK(centred(spheres[0], 2.0f));
	VOE_TEST_CHECK(centred(spheres[1], 3.0f));
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, &frame, spheres, 1), 1);

	voe_scene_transform_remember(world);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, &frame, spheres, 8), 0);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, size, capacities(ALL_PASSES), &error);
	voe_3d_shapes shapes;
	two_answers still;
	bool captures;

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
	moved_casters_mark_spheres(&shapes);
	captures = voe_render_point_shadows_ready(device);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	if (!captures)
		printf("note: no shaderOutputLayer, so nothing is captured\n");

	it_bounces(1, true, 0, captures);
	still = two_frames(VOE_RENDER_SHADOW_CASCADES, 0, true, 0, false);
	VOE_TEST_CHECK(still.first && still.second);
	VOE_TEST_CHECK_INT(still.cascades, VOE_RENDER_SHADOW_CASCADES);
	it_bounces(0, true, 1, captures);
	it_bounces(1, false, 0, captures);
	it_bounces_in_two_views(captures);
	return voe_test_result();
}
