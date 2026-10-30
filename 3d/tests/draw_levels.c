// The draws use the level the camera chooses (ADR-0425 point 5): a camera at
// the origin looks down −Z under a slanted sun that casts, and one model row
// wearing a 64 × 32 unit sphere from model_glb.inc stands on that axis,
// casting too. The frame's triangle count, every pass's, is read after a
// whole frame: the shadows call, then the view's pass and the draw system.
//
// 1. The sphere loaded with its levels file, 3 m before the camera: N.
// 2. The same 400 m away: under N ÷ 4, since there the camera chooses a level
//    far coarser than at 3 m, and the cascades draw what it chooses.
// 3. The sphere loaded with NULL levels, 400 m away: over four times check 2's
//    count. A model without levels is drawn whole at any distance.
//
// The swap's dissolve through a memory (0500 point 1, 0501), at s from the
// loaded sphere's levels[1].bound b and the lens's P[1][1] as model_levels.c
// works them, the memory's clock in seconds, each against N(s), the frame
// with no memory there:
// 4. First seen at 1.25 b: N(1.25 b), level 0 alone.
// 5. At 0.95 b, 0.1 s later the swap's first frame, 0.2 s later: at least
//    N(0.95 b), level 1 alone, plus level 0's triangles, both drawn.
// 6. Then with the row's fade 0.5: at most N(0.95 b), since a held row is
//    drawn whole at its level and not dissolved.
// 7. 0.3 s after the swap: N(0.95 b), level 1 alone, cascades included.
// 8. Back at 1.01 b, just inside the bound, two frames: the same twice and
//    under N(1.01 b), level 1 kept whole.
// 9. 10% nearer, the swap back's first frame and 0.1 s on: over N, both.
//
// Radius 1 on the camera's axis, so every distance is in view; the far plane
// is past 400 m. Nothing bounces, so no capture adds its last level. Every
// frame builds its world afresh, the model row the same entity each time, so
// one key in the memory.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include <3d/draw_system.h>
#include <3d/lods_kept.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shadow_cascades.h>
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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

// The memory's 12 MiB, and every frame's world, never rewound.
#define SCRATCH (64 * 1024 * 1024)
#define SIDE 32
#define RINGS 32
#define SEGMENTS 64
#define LEVELLED "Assets/ball.glb"
#define PLAIN "Assets/plain.glb"

// Two copies of the sphere, one with up to VOE_3D_MODEL_LEVELS levels, and one
// spare; the sphere drawn once into the view and once into each cascade; four
// shadow passes and the view's.
static const voe_render_capacities CAPACITIES = {
	.vertices = 32 * 1024,
	.indices = 128 * 1024,
	.geometries = VOE_3D_MODEL_LEVELS + 2,
	.objects = 2 * (1 + VOE_RENDER_SHADOW_CASCADES),
	.shadings = 16,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
					.near_plane = 0.1f,
					.far_plane = 1000.0f };

// The camera at the origin looking down −Z, the sun, and one model row
// wearing `path` `distance` metres before the camera at `fade`.
static voe_ecs_world *a_world(voe_base_arena *arena, const char *path,
			      double distance, float fade)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_3d_model row = { .cast_shadows = true, .fade = fade };

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, LENS));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.4f, -1.0f, 0.3f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .cast_shadows = true }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, 0.0, -distance },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// One whole frame of `path` `distance` metres away at `fade`: the triangles it
// drew, through `kept` begun at `now` seconds, none with NULL.
static uint64_t triangles(voe_base_arena *arena, voe_render_device *device,
			  const voe_3d_models *models, const char *path,
			  double distance, float fade, voe_3d_lods_kept *kept,
			  double now)
{
	voe_ecs_world *world = a_world(arena, path, distance, fade);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	bool drawing = false;
	uint64_t count;

	frame.models = models;
	if (kept != NULL)
		voe_3d_lods_kept_begin(kept, now);
	frame.lods = kept;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	count = voe_render_frame_triangle_count(device);
	printf("%s at %.2f m, fade %.1f: %llu triangles\n", path, distance,
	       fade, (unsigned long long)count);
	return count;
}

// The sphere loaded as LEVELLED with its levels file and as PLAIN without.
static void load_spheres(voe_base_arena *arena, voe_render_device *device,
			 voe_3d_models *models)
{
	voe_base_error error = VOE_BASE_OK;
	size_t glb_size;
	size_t levels_size = 0;
	const uint8_t *glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	const uint8_t *levels = voe_3d_model_levels_make(
		arena, glb, glb_size, VOE_ASSETS_LEVELS_DEFAULT,
		&levels_size, &error);

	VOE_TEST_CHECK(levels != NULL);
	VOE_TEST_CHECK(voe_3d_models_load_levelled(models, device, LEVELLED, 1,
						   glb, glb_size, levels,
						   levels_size, &error));
	VOE_TEST_CHECK(voe_3d_models_load_levelled(
		models, device, PLAIN, 1, glb, glb_size, NULL, 0, &error));
}

static void draws_use_the_level(voe_base_arena *arena,
				voe_render_device *device,
				const voe_3d_models *models)
{
	uint64_t near =
		triangles(arena, device, models, LEVELLED, 3.0, 0.0f, NULL, 0.0);
	uint64_t far =
		triangles(arena, device, models, LEVELLED, 400.0, 0.0f, NULL, 0.0);
	uint64_t whole =
		triangles(arena, device, models, PLAIN, 400.0, 0.0f, NULL, 0.0);

	VOE_TEST_CHECK(near > 0);
	VOE_TEST_CHECK(far > 0);
	VOE_TEST_CHECK(far * 4 < near);
	VOE_TEST_CHECK(whole > far * 4);
}

// 4 to 9: the distance at which s is a given value is radius × P[1][1] ÷ s,
// the sphere's centre being the origin of its row.
static void a_swap_dissolves(voe_base_arena *arena, voe_render_device *device,
			     const voe_3d_models *models)
{
	const voe_3d_model_entry *ball = voe_3d_models_find(models, LEVELLED);
	const double focal = voe_3d_projection(LENS, 1.0f).m[1][1];
	const uint64_t level_0 = SEGMENTS * (RINGS - 1) * 2;
	voe_3d_lods_kept *kept = voe_3d_lods_kept_create(arena);
	double near, past, inside, nearer;
	uint64_t first, both, fading, settled, kept_1, kept_2, back, back_both;

	VOE_TEST_CHECK(ball != NULL && ball->level_count >= 2);
	if (ball == NULL || ball->level_count < 2)
		return;
	// Level 2 swaps in below 0.9 × 1.01 b, so 1.01 b keeps level 1.
	VOE_TEST_CHECK(ball->level_count == 2 ||
		       ball->levels[2].bound < 0.85 * ball->levels[1].bound);
	near = ball->radius * focal / (1.25 * ball->levels[1].bound);
	past = ball->radius * focal / (0.95 * ball->levels[1].bound);
	inside = ball->radius * focal / (1.01 * ball->levels[1].bound);
	nearer = 0.9 * inside;

	first = triangles(arena, device, models, LEVELLED, near, 0.0f, kept, 0.0);
	(void)triangles(arena, device, models, LEVELLED, past, 0.0f, kept, 0.1);
	both = triangles(arena, device, models, LEVELLED, past, 0.0f, kept, 0.2);
	fading = triangles(arena, device, models, LEVELLED, past, 0.5f, kept, 0.3);
	settled = triangles(arena, device, models, LEVELLED, past, 0.0f, kept, 0.4);
	kept_1 = triangles(arena, device, models, LEVELLED, inside, 0.0f, kept, 0.5);
	kept_2 = triangles(arena, device, models, LEVELLED, inside, 0.0f, kept, 0.6);
	(void)triangles(arena, device, models, LEVELLED, nearer, 0.0f, kept, 0.7);
	back_both =
		triangles(arena, device, models, LEVELLED, nearer, 0.0f, kept, 0.8);
	back = triangles(arena, device, models, LEVELLED, nearer, 0.0f, NULL, 0.0);

	VOE_TEST_CHECK(first == triangles(arena, device, models, LEVELLED, near,
					  0.0f, NULL, 0.0));
	VOE_TEST_CHECK(settled == triangles(arena, device, models, LEVELLED,
					    past, 0.0f, NULL, 0.0));
	VOE_TEST_CHECK(settled > 0);
	VOE_TEST_CHECK(both >= settled + level_0);
	VOE_TEST_CHECK(fading <= settled);
	VOE_TEST_CHECK(kept_1 == kept_2);
	VOE_TEST_CHECK(kept_1 < triangles(arena, device, models, LEVELLED,
					  inside, 0.0f, NULL, 0.0));
	VOE_TEST_CHECK(back_both > back);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_models *models;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	models = voe_3d_models_new();
	load_spheres(arena, device, models);

	draws_use_the_level(arena, device, models);
	a_swap_dissolves(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
