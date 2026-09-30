// Each live particle is drawn (ADR-0298 point 6): an emitter of burst 5, run
// once through the emitter system with the soft dot loaded, is five more draws
// than the same world without it; with no store it is none; and a glowing one
// draws with part 1's shading, the unlit glow.
//
// DRAWS ARE COUNTED AS 3d/tests/draw_system.c COUNTS THEM: every draw is one
// command, and voe_render_frame_draw_count is what a frame issued. The worlds
// here hold nothing else drawable, so a particle drawn is a command more.
//
// THE SHADING IS READ OFF THE PICTURE, THROUGH voe_render_target_read. The world
// has no light, and a world with none draws lit surfaces black (ADR-0287), so a
// lit particle (part 0) is dark at the centre of the picture and a glowing one
// (part 1, unlit) is the dot's white there.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_system.c does.
#include <3d/draw_system.h>
#include <3d/emitter_component.h>
#include <3d/emitter_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
#define STEP (1.0f / 60.0f)

// The dot's quad and two records, and room for the five particles.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 8,
	.shadings = 4,
	.passes = 1,
};

// A world with every table the draw walks, a camera at the origin looking along
// -Z and no light; `emitters` says whether the emitter tables are registered.
static voe_ecs_world *a_world(voe_base_arena *arena, bool emitters)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 12,
		.intent_types = 12,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity eye = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);
	if (emitters)
		voe_3d_emitter_register(world, 2);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, eye,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, eye,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	return world;
}

// An emitter three metres in front of the camera, stopped, bursting `burst`
// still, white, opaque particles a metre across, run once so they are live.
static void add_an_emitter(voe_ecs_world *world, uint32_t burst, float glow)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_emitter emitter = {
		.burst = burst,
		.life = 10.0f,
		.direction = { 0.0f, 1.0f, 0.0f },
		.size_start = 1.0f,
		.size_end = 1.0f,
		.colour_start = { 1.0f, 1.0f, 1.0f },
		.colour_end = { 1.0f, 1.0f, 1.0f },
		.alpha_start = 1.0f,
		.alpha_end = 1.0f,
		.glow = glow,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 0.0, 0.0, -3.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_emitter_add(world, entity, emitter));
	VOE_TEST_CHECK(voe_3d_emitter_control_submit(
		world, (voe_3d_emitter_control){ .entity = entity,
						 .kind = VOE_3D_EMITTER_BURST }));
	voe_3d_emitter_system_run(world, STEP);
	VOE_TEST_CHECK_INT(voe_3d_particles_get(world, entity)->count, burst);
}

// One frame of `world` drawn from `models`: how many commands it issued, and its
// picture in `picture` when that is not NULL.
static uint32_t a_frame(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, const voe_3d_models *models,
			voe_render_picture *picture)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera = { .view = frame.view,
					  .light = frame.light };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	uint32_t drawn = 0;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	drawn = voe_render_frame_draw_count(device);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	if (picture != NULL)
		VOE_TEST_CHECK(voe_render_target_read(device,
						      VOE_RENDER_TARGET_WINDOW,
						      arena, picture, &error));
	return drawn;
}

// Five particles are five draws more than no emitter, and none with no store;
// a world that never registered emitters draws as before.
static void each_live_particle_is_one_draw(voe_base_arena *arena,
					   voe_render_device *device,
					   const voe_3d_models *models)
{
	voe_ecs_world *without = a_world(arena, false);
	voe_ecs_world *with = a_world(arena, true);
	uint32_t base = a_frame(without, device, arena, models, NULL);

	add_an_emitter(with, 5, 0.0f);
	VOE_TEST_CHECK_INT(a_frame(with, device, arena, models, NULL), base + 5);
	VOE_TEST_CHECK_INT(a_frame(with, device, arena, NULL, NULL), base);
}

// The red channel of the picture's centre pixel.
static uint8_t centre_red(voe_render_picture picture)
{
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels == NULL)
		return 0;
	return picture.pixels[((size_t)(picture.height / 2) * picture.width +
			       picture.width / 2) *
			      4];
}

// With no light, a lit particle is dark at the centre and a glowing one white:
// the glow is part 1, the unlit record.
static void a_glowing_one_is_unlit(voe_base_arena *arena,
				   voe_render_device *device,
				   const voe_3d_models *models)
{
	voe_ecs_world *lit = a_world(arena, true);
	voe_ecs_world *glowing = a_world(arena, true);
	voe_render_picture picture = { 0 };
	uint8_t dark;
	uint8_t bright;

	add_an_emitter(lit, 1, 0.0f);
	add_an_emitter(glowing, 1, 1.0f);
	VOE_TEST_CHECK_INT(a_frame(lit, device, arena, models, &picture), 1);
	dark = centre_red(picture);
	VOE_TEST_CHECK_INT(a_frame(glowing, device, arena, models, &picture), 1);
	bright = centre_red(picture);
	VOE_TEST_CHECK(bright > 200);
	VOE_TEST_CHECK(dark < bright / 2);
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
	VOE_TEST_CHECK(voe_3d_models_load_dot(models, device, &error));

	each_live_particle_is_one_draw(arena, device, models);
	a_glowing_one_is_unlit(arena, device, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
