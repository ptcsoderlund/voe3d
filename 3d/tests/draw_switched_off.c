// A switched-off owner is left out of every draw (0427, 0428 point 3): a
// camera at the origin looks down −Z under a slanted sun that casts, and a
// parent entity's child wears the 64 × 32 unit sphere from model_glb.inc 3 m
// before it, casting too. Both have an identity. The frame's triangle count,
// every pass's, is read after a whole frame: the shadows call, then the view's
// pass and the draw system.
//
// 1. A world with no model row: B.
// 2. The parent and its child, both on: N, over B.
// 3. The child's identity submitted off and the system run: B.
// 4. The child back on and the parent off: B, the child off through it.
// 5. The parent back on: N, the child's own switch on as it was left.
//
// EVERY COUNT IS COMPARED TO B AND NOT TO NOUGHT, because a frame draws more
// than its model rows; an off model is as if it were not there, so its frame
// counts what a frame that never had the row counts, shadow passes included.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_levels.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "model_glb.inc"

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 32
#define RINGS 32
#define SEGMENTS 64
#define SPHERE "Assets/ball.glb"

// One sphere, drawn once into the view and once into each cascade; four
// shadow passes and the view's.
static const voe_render_capacities CAPACITIES = {
	.vertices = 32 * 1024,
	.indices = 128 * 1024,
	.geometries = 8,
	.objects = 2 * (1 + VOE_RENDER_SHADOW_CASCADES),
	.shadings = 16,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

static const voe_scene_transform STILL = { .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
					   .scale = { 1.0f, 1.0f, 1.0f } };

// The camera at the origin looking down −Z, and the sun.
static voe_ecs_world *a_world(voe_base_arena *arena)
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
	voe_scene_transform sun = STILL;

	voe_scene_transform_register(world, 4);
	voe_scene_parent_register(world, 4);
	voe_scene_identity_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, STILL));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 1000.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	sun.rotation = voe_scene_light_facing((voe_math_float3){ 0.4f, -1.0f, 0.3f });
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, sun));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .cast_shadows = true }));
	return world;
}

// A parent at the origin and, under it, a child wearing the sphere 3 m before
// the camera, each with an identity, on. The child into `child`.
static voe_ecs_entity add_the_pair(voe_ecs_world *world, voe_ecs_entity *child)
{
	voe_ecs_entity parent = { 0 };
	voe_scene_transform placed = STILL;
	voe_3d_model row = { .cast_shadows = true };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(world, parent, STILL));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, parent, (voe_scene_identity){ .id = 1, .name = "Parent" }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, child));
	placed.position = (voe_math_double3){ 0.0, 0.0, -3.0 };
	VOE_TEST_CHECK(voe_scene_transform_add(world, *child, placed));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, *child, (voe_scene_identity){ .id = 2, .name = "Child" }));
	snprintf(row.path, sizeof(row.path), "%s", SPHERE);
	VOE_TEST_CHECK(voe_3d_model_add(world, *child, row));

	VOE_TEST_CHECK(voe_scene_parent_set(world, *child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	return parent;
}

// `entity`'s identity submitted with `off` and the identity system run.
static void switch_to(voe_ecs_world *world, voe_ecs_entity entity, bool off)
{
	voe_scene_identity identity = *voe_scene_identity_get(world, entity);

	identity.off = off;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = entity,
						    .identity = identity }));
	voe_scene_identity_system_run(world);
	VOE_TEST_CHECK(voe_scene_identity_get(world, entity)->off == off);
}

// One whole frame of `world`: the triangles it drew.
static uint64_t triangles(voe_base_arena *arena, voe_render_device *device,
			  const voe_3d_models *models, voe_ecs_world *world,
			  const char *what)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	bool drawing = false;
	uint64_t count;

	frame.models = models;
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
	printf("%s: %llu triangles\n", what, (unsigned long long)count);
	return count;
}

static void off_is_not_drawn(voe_base_arena *arena, voe_render_device *device,
			     const voe_3d_models *models)
{
	uint64_t b = triangles(arena, device, models, a_world(arena),
			       "no model row");
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity child = { 0 };
	voe_ecs_entity parent = add_the_pair(world, &child);
	uint64_t n = triangles(arena, device, models, world, "both on");
	uint64_t counted;

	VOE_TEST_CHECK(n > b);

	switch_to(world, child, true);
	counted = triangles(arena, device, models, world, "child off");
	VOE_TEST_CHECK_INT((long long)counted, (long long)b);

	switch_to(world, child, false);
	switch_to(world, parent, true);
	counted = triangles(arena, device, models, world, "parent off");
	VOE_TEST_CHECK_INT((long long)counted, (long long)b);

	switch_to(world, parent, false);
	counted = triangles(arena, device, models, world, "parent on");
	VOE_TEST_CHECK_INT((long long)counted, (long long)n);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_3d_models *models;
	size_t glb_size;
	const uint8_t *glb;

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
	glb = sphere_glb(arena, RINGS, SEGMENTS, false, &glb_size);
	VOE_TEST_CHECK(voe_3d_models_load_levelled(models, device, SPHERE, 1, glb,
						   glb_size, NULL, 0, &error));

	off_is_not_drawn(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
