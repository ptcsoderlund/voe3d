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
// THE PASS COUNTS, TWO FRAMES ON ONE DEVICE. The world fits a 2 m level grid,
// so the 1 m nest begins beside it (0389). With the sun at bounces 1, frame
// one only asks for the volumes and opens no capture pass: true on the four
// cascades' passes alone. Frame two captures four times, shared, and draws each
// volume's sun map (0329): true with exactly four cascades, four capture
// passes and two bounce shadow passes, false with one fewer. At bounces 0 and no
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
// two sun maps (0330), and the capture passes the frame's budget of four gives,
// all the first view's: true on exactly those, false on one fewer.
//
// THE NESTS (0389 points 1 to 3): after two frames volumes 0 and 3 are placed,
// the 1 m nest at its home about the eye, and 1 and 2 are not. Frame two's
// breakdown has capture passes 1 to 3 before the level grid's relight and the
// fourth, volume 3's, after it; a card without timestamps says so.
//
// THE STALE SPHERES, each the caster's own size (0389 point 7); the wall's
// radius is half its 0.2 × 2 × 4 world box's diagonal. Remembered and moved a
// metre along X, it marks two, at (2, 0, -5) and (3, 0, -5) about the eye; a
// room of one gives one; remembered again none, nor half a millimetre on.
// Stretched to 4 m tall it marks two at (2, 0, -5), the old radius and the new.
// A unit cube added after the remember marks one where it is, of half √3. With
// the shape changes table and no previous table, the wall recoloured marks one
// where it is, the next run none. Without a previous table a moved wall and a
// new cube mark none. The ground never changes and marks nothing throughout.
//
// THE BOX OF THE STILL CASTERS (0332 point 1). A 40 × 0.1 × 40 ground at
// y −0.05 and a 2 m cube at (5, 1, 0), the built-in cube a unit one, box
// (−20, −0.1, −20) to (20, 2, 20) within 1e-4, the same with the eye at the
// origin and at (10 km, 5, −10 km). The cube turned 45° about y, the ground
// hidden, spans 5 ± √2 in x. A cube moved this step leaves the ground's box, a
// ground with `cast_shadows` false the cube's, and both moved answer false
// with the corners untouched.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/shadows.c does.
#include "../src/draw_bounce.h"

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
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>
#include <math/quat.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32

// The volumes this world begins: the level grid and the 1 m nest.
#define BEGUN 2

// The passes of one view's four cascades, four capture passes and a bounce
// shadow pass for each of the VOE_RENDER_BOUNCE_VOLUMES volumes.
#define ALL_PASSES                                                     \
	(VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES + \
	 VOE_RENDER_BOUNCE_VOLUMES)

// The two casters, two objects each, drawn into every pass of two views; point
// shadows sized so their readiness says whether the card has shaderOutputLayer.
static voe_render_capacities capacities(uint32_t passes)
{
	return (voe_render_capacities){
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2 * 2 * 2 * ALL_PASSES,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = passes,
		.targets = 1,
		.shadow_size = VOE_3D_SHADOW_TEXELS,
		.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
	};
}

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
// on the cascades', the capture passes and each begun volume's sun map when the
// sun bounces and casts, and false on one fewer; without, true on the
// cascades' passes.
static void it_bounces(uint32_t bounces, bool casts, uint32_t lamp_bounces,
		       bool captures)
{
	uint32_t cascades = casts ? VOE_RENDER_SHADOW_CASCADES : 0;
	uint32_t wanted = cascades + VOE_RENDER_BOUNCE_CAPTURE_PASSES +
			  (casts && bounces >= 1 ? BEGUN : 0);
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
// true on both views' cascades and each begun volume's sun map and the frame's
// capture passes and false on one fewer; without shaderOutputLayer true on the
// cascades' alone.
static void it_bounces_in_two_views(bool captures)
{
	uint32_t cascades = 2 * VOE_RENDER_SHADOW_CASCADES;
	uint32_t wanted = cascades + VOE_RENDER_BOUNCE_CAPTURE_PASSES + 2 * BEGUN;
	two_answers enough =
		two_frames(captures ? wanted : cascades, 1, true, 0, true);

	VOE_TEST_CHECK(enough.first && enough.second);
	if (captures)
		VOE_TEST_CHECK(!two_frames(wanted - 1, 1, true, 0, true).second);
}

// A fresh device of ALL_PASSES in `arena`, `shapes` uploaded to it; NULL when
// none could be made, which the caller has already skipped for.
static voe_render_device *a_device(voe_base_arena *arena, voe_3d_shapes *shapes)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, size, capacities(ALL_PASSES), &error);

	VOE_TEST_CHECK(device != NULL);
	if (device != NULL)
		VOE_TEST_CHECK(voe_3d_shapes_upload(device, shapes, &error));
	return device;
}

// One frame of the shadows call for the window; its answer.
static bool a_frame(voe_ecs_world *world, voe_render_device *device,
		    voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	bool drawing = false;
	bool answer = false;

	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		answer = voe_3d_draw_system_shadows(world, device, &frame);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	return answer;
}

// The world's still casters fit a 2 m level grid; after two frames volume 0
// and volume 3, the 1 m nest at its home about the eye, are placed, and the
// 16 and 4 m nests are not. Without shaderOutputLayer nothing is placed.
static void a_two_metre_level_begins_only_the_one_metre_nest(bool captures)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_double3 min = { 0 };
	voe_math_double3 max = { 0 };
	int32_t cell[3] = { 0 };
	voe_3d_shapes shapes;
	voe_render_device *device = a_device(arena, &shapes);
	voe_ecs_entity wall;
	voe_ecs_world *world;
	voe_3d_frame frame;
	voe_3d_bounce_grid home;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	world = a_world(arena, &shapes, false, 1, true, 0, &wall);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	VOE_TEST_CHECK(voe_3d_bounce_box(world, device, &frame, &min, &max));
	VOE_TEST_CHECK(voe_3d_bounce_grid_fit(min, max, frame.eye).spacing == 2.0f);
	VOE_TEST_CHECK(a_frame(world, device, arena));
	VOE_TEST_CHECK(a_frame(world, device, arena));
	VOE_TEST_CHECK(voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						0, cell) == captures);
	VOE_TEST_CHECK(!voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						 1, cell));
	VOE_TEST_CHECK(!voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						 2, cell));
	VOE_TEST_CHECK(voe_render_bounce_placed(device, VOE_RENDER_TARGET_WINDOW,
						3, cell) == captures);
	home = voe_3d_bounce_grid_nest(voe_3d_bounce_nest_spacing(2), NULL,
				       frame.eye);
	if (captures)
		VOE_TEST_CHECK(cell[0] == home.cell[0] && cell[1] == home.cell[1] &&
			       cell[2] == home.cell[2]);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

// The first pass in `times` named `name`, or `count` when none is.
static uint32_t pass_at(const voe_render_pass_time *times, uint32_t count,
			const char *name)
{
	for (uint32_t i = 0; i < count; i++)
		if (strcmp(times[i].name, name) == 0)
			return i;
	return count;
}

// The level grid captures for many frames; frame two's breakdown, the first
// to hold a capture, has three capture passes before the level grid's relight
// and the fourth, volume 3's, after it. A card without timestamps says so.
static void the_finest_nest_keeps_a_capture_pass(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_render_pass_time times[32];
	uint32_t count = 0;
	uint32_t relight;
	voe_3d_shapes shapes;
	voe_render_device *device = a_device(arena, &shapes);
	voe_ecs_entity wall;
	voe_ecs_world *world;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	world = a_world(arena, &shapes, false, 1, true, 0, &wall);
	for (uint32_t frames = 0;
	     frames < 8 && pass_at(times, count, "bounce capture 1") == count;
	     frames++) {
		VOE_TEST_CHECK(a_frame(world, device, arena));
		count = voe_render_frame_pass_times(device, times, 32);
	}
	relight = pass_at(times, count, "bounce relight");
	if (pass_at(times, count, "bounce capture 1") == count)
		printf("note: no timestamps, so no breakdown to read\n");
	else
		VOE_TEST_CHECK(pass_at(times, count, "bounce capture 3") < relight &&
			       relight < pass_at(times, count, "bounce capture 4") &&
			       pass_at(times, count, "bounce capture 4") < count);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

// Half the diagonal of a box of sides `x`, `y` and `z`.
static float half_diagonal(float x, float y, float z)
{
	return 0.5f * sqrtf(x * x + y * y + z * z);
}

// The wall's world bounding radius: half its 0.2 × 2 × 4 box's diagonal.
#define WALL_RADIUS half_diagonal(0.2f, 2.0f, 4.0f)

// Whether `sphere` is centred at `x`, 0, -5 with radius `w`, within 1e-4.
static bool centred(voe_math_float4 sphere, float x, float w)
{
	printf("sphere %g %g %g r %g\n", sphere.x, sphere.y, sphere.z, sphere.w);
	return fabsf(sphere.x - x) < 1e-4f && fabsf(sphere.y) < 1e-4f &&
	       fabsf(sphere.z + 5.0f) < 1e-4f && fabsf(sphere.w - w) < 1e-4f;
}

// `entity`'s transform made `placed` this step.
static void place(voe_ecs_world *world, voe_ecs_entity entity,
		  voe_scene_transform placed)
{
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ entity, placed }));
	voe_scene_transform_system_run(world);
}

// A metre's move marks two of the wall's radius, a room of one one; remembered
// again none, and half a millimetre none.
static void moved_casters_mark_spheres(const voe_render_device *device,
				       const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, true, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_scene_transform moved;

	voe_scene_transform_remember(world);
	moved = *voe_scene_transform_get(world, wall);
	moved.position.x += 1.0;
	place(world, wall, moved);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 2);
	VOE_TEST_CHECK(centred(spheres[0], 2.0f, WALL_RADIUS));
	VOE_TEST_CHECK(centred(spheres[1], 3.0f, WALL_RADIUS));
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 1), 1);

	voe_scene_transform_remember(world);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	moved.position.x += 0.0005;
	place(world, wall, moved);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	voe_base_arena_destroy(arena);
}

// The wall stretched to 4 m tall marks two where it stands, its old radius
// and its new one.
static void a_scaled_caster_marks(const voe_render_device *device,
				  const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, true, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_scene_transform scaled;

	voe_scene_transform_remember(world);
	scaled = *voe_scene_transform_get(world, wall);
	scaled.scale.y = 4.0f;
	place(world, wall, scaled);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 2);
	VOE_TEST_CHECK(centred(spheres[0], 2.0f, WALL_RADIUS));
	VOE_TEST_CHECK(centred(spheres[1], 2.0f, half_diagonal(0.2f, 4.0f, 4.0f)));
	voe_base_arena_destroy(arena);
}

// A unit cube added after the remember marks one where it is, of its own
// radius; remembered, none.
static void a_new_caster_marks_where_it_is(const voe_render_device *device,
					   const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, true, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);

	voe_scene_transform_remember(world);
	(void)add_a_shape(world, (voe_math_double3){ -3.0, 0.0, -5.0 },
			  (voe_math_float3){ 1.0f, 1.0f, 1.0f },
			  (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 1);
	VOE_TEST_CHECK(centred(spheres[0], -3.0f, half_diagonal(1.0f, 1.0f, 1.0f)));

	voe_scene_transform_remember(world);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	voe_base_arena_destroy(arena);
}

// The wall turned blue in a world with the changes table and no previous one
// marks one where it is, of its radius; the next run, none.
static void a_recoloured_shape_marks_where_it_is(const voe_render_device *device,
						 const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, false, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);

	voe_3d_shape_changes_register(world, 8);
	VOE_TEST_CHECK(voe_3d_shape_submit(
		world, (voe_3d_shape_intent){
			       .entity = wall,
			       .shape = { .kind = VOE_3D_SHAPE_CUBE,
					  .colour = { 0.0f, 0.0f, 1.0f },
					  .cast_shadows = true } }));
	voe_3d_shape_system_run(world, shapes);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 1);
	VOE_TEST_CHECK(centred(spheres[0], 2.0f, WALL_RADIUS));

	voe_3d_shape_system_run(world, shapes);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	voe_base_arena_destroy(arena);
}

// With no previous table, a metre's move and a new cube mark nothing.
static void a_world_without_a_previous_table_marks_no_move(
	const voe_render_device *device, const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_math_float4 spheres[8] = { 0 };
	voe_ecs_entity wall;
	voe_ecs_world *world = a_world(arena, shapes, false, 1, true, 0, &wall);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_scene_transform moved = *voe_scene_transform_get(world, wall);

	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	moved.position.x += 1.0;
	place(world, wall, moved);
	(void)add_a_shape(world, (voe_math_double3){ -3.0, 0.0, -5.0 },
			  (voe_math_float3){ 1.0f, 1.0f, 1.0f },
			  (voe_math_float3){ 1.0f, 1.0f, 1.0f });
	voe_3d_shape_system_run(world, shapes);
	VOE_TEST_CHECK_INT(voe_3d_bounce_stale(world, device, &frame, spheres, 8), 0);
	voe_base_arena_destroy(arena);
}

// A cube caster `at`, scaled by `scale` and turned `turn` radians about y,
// casting when `casts`; the entity.
static voe_ecs_entity add_a_caster(voe_ecs_world *world, voe_math_double3 at,
				   voe_math_float3 scale, float turn, bool casts)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = at,
			.rotation = voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f }, turn),
			.scale = scale }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 1.0f, 1.0f, 1.0f },
				.cast_shadows = casts }));
	return entity;
}

// The camera, the ground into `ground`, casting when `ground_casts`, and the
// cube turned `turn` into `cube`, every transform remembered.
static voe_ecs_world *a_level(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      float turn, bool ground_casts,
			      voe_ecs_entity *ground, voe_ecs_entity *cube)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity camera = { 0 };

	voe_scene_transform_register(world, 4);
	voe_scene_transform_previous_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_shape_register(world, 4);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, camera,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, camera,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	*ground = add_a_caster(world, (voe_math_double3){ 0.0, -0.05, 0.0 },
			       (voe_math_float3){ 40.0f, 0.1f, 40.0f }, 0.0f,
			       ground_casts);
	*cube = add_a_caster(world, (voe_math_double3){ 5.0, 1.0, 0.0 },
			     (voe_math_float3){ 2.0f, 2.0f, 2.0f }, turn, true);
	voe_3d_shape_system_run(world, shapes);
	voe_scene_transform_remember(world);
	return world;
}

// Moves `entity` a metre along x this step.
static void step_aside(voe_ecs_world *world, voe_ecs_entity entity)
{
	voe_scene_transform moved = *voe_scene_transform_get(world, entity);

	moved.position.x += 1.0;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ entity, moved }));
	voe_scene_transform_system_run(world);
}

// Whether the still casters' box is `low` to `high` within 1e-4.
static bool boxed(const voe_ecs_world *world, const voe_render_device *device,
		  const voe_3d_frame *frame, voe_math_double3 low,
		  voe_math_double3 high)
{
	voe_math_double3 min = { 0 };
	voe_math_double3 max = { 0 };

	if (!voe_3d_bounce_box(world, device, frame, &min, &max))
		return false;
	printf("box %g %g %g to %g %g %g\n", min.x, min.y, min.z, max.x, max.y,
	       max.z);
	return fabs(min.x - low.x) < 1e-4 && fabs(min.y - low.y) < 1e-4 &&
	       fabs(min.z - low.z) < 1e-4 && fabs(max.x - high.x) < 1e-4 &&
	       fabs(max.y - high.y) < 1e-4 && fabs(max.z - high.z) < 1e-4;
}

// The level's box at two eyes, turned, with a caster moved, not casting,
// hidden, and with none still.
static void still_casters_box_the_level(const voe_render_device *device,
					const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	const double r = sqrt(2.0);
	voe_math_double3 untouched = { 7.0, 7.0, 7.0 };
	voe_math_double3 also = untouched;
	voe_ecs_entity ground, cube;
	voe_ecs_world *world = a_level(arena, shapes, 0.0f, true, &ground, &cube);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);

	VOE_TEST_CHECK(boxed(world, device, &frame, (voe_math_double3){ -20, -0.1, -20 },
			     (voe_math_double3){ 20, 2, 20 }));
	frame.eye = (voe_math_double3){ 10000.0, 5.0, -10000.0 };
	VOE_TEST_CHECK(boxed(world, device, &frame, (voe_math_double3){ -20, -0.1, -20 },
			     (voe_math_double3){ 20, 2, 20 }));

	world = a_level(arena, shapes, 0.78539816f, true, &ground, &cube);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	frame.hidden = ground;
	VOE_TEST_CHECK(boxed(world, device, &frame, (voe_math_double3){ 5 - r, 0, -r },
			     (voe_math_double3){ 5 + r, 2, r }));

	world = a_level(arena, shapes, 0.0f, false, &ground, &cube);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	VOE_TEST_CHECK(boxed(world, device, &frame, (voe_math_double3){ 4, 0, -1 },
			     (voe_math_double3){ 6, 2, 1 }));

	world = a_level(arena, shapes, 0.0f, true, &ground, &cube);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	step_aside(world, cube);
	VOE_TEST_CHECK(boxed(world, device, &frame, (voe_math_double3){ -20, -0.1, -20 },
			     (voe_math_double3){ 20, 0, 20 }));
	step_aside(world, ground);
	VOE_TEST_CHECK(!voe_3d_bounce_box(world, device, &frame, &untouched, &also));
	VOE_TEST_CHECK(untouched.x == 7.0 && also.z == 7.0);
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
	moved_casters_mark_spheres(device, &shapes);
	a_scaled_caster_marks(device, &shapes);
	a_new_caster_marks_where_it_is(device, &shapes);
	a_recoloured_shape_marks_where_it_is(device, &shapes);
	a_world_without_a_previous_table_marks_no_move(device, &shapes);
	still_casters_box_the_level(device, &shapes);
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
	a_two_metre_level_begins_only_the_one_metre_nest(captures);
	if (captures)
		the_finest_nest_keeps_a_capture_pass();
	return voe_test_result();
}
