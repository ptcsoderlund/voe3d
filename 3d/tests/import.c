// The import, end to end: a `.glb`'s bytes in, entities with transforms, meshes
// and materials out — and then one frame drawn from them, so that the whole
// chain from a file to a submitted command buffer is exercised once.
//
// THE FILE IS IN model_data.inc AND EVERY NUMBER IN IT IS THERE FOR A REASON:
//
//   - Two nodes, one the other's child, and the parent has a rotation as well as
//     a translation. That is what makes the flattening checkable: the child's
//     world transform is only right if the parent's rotation carried it, and a
//     composition done the other way round puts it somewhere else entirely.
//   - The child's mesh has two primitives with two different materials, so one
//     node becomes two entities — the case a reader that assumed one mesh is one
//     draw gets wrong.
//   - Both materials point at two different glTF textures whose source is the
//     same picture. Both entities' materials therefore have to hold the same
//     texture id, and the file has to have cost exactly one texture slot.
//
// THE MIRRORED MODEL IS WHAT THIS FILE IS REALLY LOOKING FOR. A world matrix is
// decomposed into a position, a rotation and a scale on the way into the
// transform component, and a quaternion read out of a matrix with its signs
// flipped describes the opposite rotation — which draws a model that is mirrored
// and otherwise perfect. So the check is not on the position alone: the
// transform is composed back into a matrix and a known point is put through it.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because uploading a picture and a
// mesh needs one. A machine with no Vulkan and a card too old for what the
// engine requires are both the build box rather than this engine being wrong;
// anything else is a driver that refused something, and that is a failure.
#include <3d/draw_system.h>
#include <3d/panel_component.h>
#include <3d/import.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>

#include "model_data.inc"

// Room for the decoded picture, the model's arrays, the JSON tokens and the
// world. A round number well above what this file needs.
#define SCRATCH (4 * 1024 * 1024)

#define SIDE 64
#define TOLERANCE 1e-5f

static const voe_render_capacities CAPACITIES = {
	.vertices = 256,
	.indices = 256,
	.geometries = 8,
	.objects = 8,
	.shadings = 8,
	.passes = 1,
};

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	// Nothing here builds a panel, and the table is still registered: the
	// draw system walks it every frame, so registering it is part of
	// building a world that can be drawn at all — see 3d/draw_system.h.
	voe_3d_panel_register(world, 16);
	return world;
}

// One camera, because the draw system needs exactly one. Its numbers are
// whatever will see something: this file checks that a frame is recorded and
// submitted, not what it looks like — render/tests/offscreen.c is what reads
// pixels.
static void add_a_camera(voe_ecs_world *world)
{
	voe_ecs_entity eye = { 0 };
	voe_scene_transform pose = {
		.position = { 0.0f, 2.0f, 6.0f },
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

// One sun, because the draw system needs exactly one. Straight down and white:
// this file checks that a frame is recorded and submitted, not what it looks
// like — render/tests/offscreen.c is what reads pixels.
static void add_the_sun(voe_ecs_world *world)
{
	voe_ecs_entity sun = { 0 };
	voe_scene_light light = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.colour = { 1.0f, 1.0f, 1.0f },
		.intensity = 1.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, light));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_ecs_world *world;
	voe_base_error error = VOE_BASE_OK;
	voe_3d_import imported = { 0 };
	const voe_3d_material *first_material;
	const voe_3d_material *second_material;
	const voe_3d_mesh *first_mesh;
	const voe_3d_mesh *second_mesh;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	world = a_world(arena);

	VOE_TEST_CHECK(voe_3d_import_glb(world, device, arena,
					 TWO_PRIMITIVES_GLB,
					 sizeof(TWO_PRIMITIVES_GLB), &imported,
					 &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);

	// One node with a two-primitive mesh, so two entities; one picture, so
	// one texture slot; two glTF materials and two primitives.
	VOE_TEST_CHECK_INT(imported.entity_count, 2);
	VOE_TEST_CHECK_INT(imported.texture_count, 1);
	VOE_TEST_CHECK_INT(imported.material_count, 2);
	VOE_TEST_CHECK_INT(imported.geometry_count, 2);

	// Two entities in the world and no more: the parent node carries only a
	// transform, so it was walked and not placed. The camera is added
	// further down, after everything about the import has been checked, so
	// that this count is the import's own.
	VOE_TEST_CHECK_INT(voe_ecs_entity_count(world), 2);
	VOE_TEST_CHECK_INT(voe_3d_mesh_count(world), 2);
	VOE_TEST_CHECK_INT(voe_3d_material_count(world), 2);
	VOE_TEST_CHECK_INT(voe_scene_transform_count(world), 2);

	if (imported.entity_count != 2) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// ---- the flattening, and the mirror it would hide
	//
	// The file's parent is at (1,0,0) turned a quarter turn about +Y; its
	// child is at (0,2,0) scaled by two. So the child's own origin lands at
	// (1,2,0) — the parent's rotation does not move a point on its own axis
	// — and the child's local +X, doubled and then turned, lands at
	// (1,2,-2), because a positive rotation about +Y takes +Z towards +X and
	// therefore +X towards -Z.
	for (uint32_t i = 0; i < imported.entity_count; i++) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, imported.entities[i]);
		voe_math_float4x4 matrix;

		VOE_TEST_CHECK(transform != NULL);
		if (transform == NULL)
			continue;

		check_vector(voe_math_double3_to_float3(transform->position),
			     (voe_math_float3){ 1.0f, 2.0f, 0.0f });
		check_vector(transform->scale,
			     (voe_math_float3){ 2.0f, 2.0f, 2.0f });

		matrix = voe_scene_transform_matrix(
			*transform, (voe_math_double3){ 0.0, 0.0, 0.0 });
		check_vector(voe_math_float4x4_transform_point(
				     matrix,
				     (voe_math_float3){ 1.0f, 0.0f, 0.0f }),
			     (voe_math_float3){ 1.0f, 2.0f, -2.0f });
	}

	// ---- the shared picture, which is the deduplication
	first_material = voe_3d_material_get(world, imported.entities[0]);
	second_material = voe_3d_material_get(world, imported.entities[1]);
	VOE_TEST_CHECK(first_material != NULL);
	VOE_TEST_CHECK(second_material != NULL);
	if (first_material != NULL && second_material != NULL) {
		// Two textures, one picture, one id — and it is a real id and
		// not the "there isn't one" slot.
		VOE_TEST_CHECK(first_material->base_colour_texture.index !=
			       VOE_RENDER_NO_TEXTURE);
		VOE_TEST_CHECK_INT(first_material->base_colour_texture.index,
				   second_material->base_colour_texture.index);
		VOE_TEST_CHECK_INT(
			first_material->base_colour_texture.generation,
			second_material->base_colour_texture.generation);

		// Two materials, though: the primitives wear different colours
		// and hold different shading records.
		VOE_TEST_CHECK(first_material->shading.index !=
			       second_material->shading.index);
		VOE_TEST_CHECK_FLOAT(first_material->base_colour.x, 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(first_material->base_colour.y, 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(second_material->base_colour.x, 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(second_material->base_colour.y, 1.0f, 0.0f);
	}

	// ---- two primitives, two ranges in the pools
	first_mesh = voe_3d_mesh_get(world, imported.entities[0]);
	second_mesh = voe_3d_mesh_get(world, imported.entities[1]);
	VOE_TEST_CHECK(first_mesh != NULL);
	VOE_TEST_CHECK(second_mesh != NULL);
	if (first_mesh != NULL && second_mesh != NULL)
		VOE_TEST_CHECK(first_mesh->geometry.index !=
			       second_mesh->geometry.index);

	// ---- and the whole chain draws
	//
	// A headless device records and submits and presents nothing, so this
	// says the tables became draws and the driver accepted them. What the
	// picture looks like is render/tests/offscreen.c's question.
	add_a_camera(world);
	add_the_sun(world);

	// Twice, so that the second frame slot is used as well — which is where
	// a per-slot buffer that was written into the wrong slot shows up. The
	// loop's shape, as 3d/draw_system.h has it: the frame's inputs, begin,
	// a pass with the frame's camera, the walk, end.
	for (int lap = 0; lap < 2; lap++) {
		voe_3d_frame frame = voe_3d_draw_system_frame(world, size);
		voe_render_pass_camera camera = { .view = frame.view,
						  .light = frame.light };
		bool drawing = false;

		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (!drawing)
			break;
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	// A window with no area: the frame's inputs are still computed without
	// dividing by zero on the way to an aspect ratio, and _begin says there
	// is nothing to draw into — so the loop runs nothing and ends nothing.
	{
		voe_platform_size none = { 0, 0 };
		bool drawing = true;

		// Computed and not used, which is the claim: no pass opens on
		// a frame with nothing to draw into, so nothing reads it.
		(void)voe_3d_draw_system_frame(world, none);
		VOE_TEST_CHECK(voe_render_frame_begin(device, none, &drawing));
		VOE_TEST_CHECK(!drawing);
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
