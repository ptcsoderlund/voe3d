// Bug 01 end to end: the tank game's scene drawn through the calls the editor
// and the game make (frame, shadows call, view's pass, run), not a hand-fitted
// grid, so the fit, the casters and the update are 3d's own.
//
// THE SCENE, lit as examples/tank_game/main.scene: beige ground (a flat box
// 60 m wide, top at y 0, base 0.8, 0.7, 0.5), a red box 2 m a side at
// (12, 1, -6) and a green one 6 m from it at (6, 1, -6); a sun in
// (0.95, 0.71, 0.71) shining down at 45 degrees along -x onto the red box's
// +x face, fill 0.09, bounces 1 (0319). The eye stands 8 m up and 14 m back
// (+z) from the red
// box, looking at its foot.
//
// TWO SUNS, 1 AND π (0310), each its own world, not the project's 9: with no
// tone map yet a sun of 9 clips the lit ground to white, and a clipped pixel
// shows no tint; that is a work order after 046. The bounce's strength is
// render's gain, VOE_BOUNCE_GAIN (0311), not this test's.
//
// TEN FRAMES of: frame begin, voe_3d_draw_system_shadows with `frame.target`
// the window, the view's pass and the run, frame end; then the window read.
//
// THE TINT (0310): the ground 1 m out from the red box's lit face,
// (14, 0, -6), reads at least 12/255 more red, in 8-bit, than the ground 8 m
// from both boxes, (12, 0, -14).
//
// NO SPOTS, at both suns: at a 7 x 7 lattice of ground points about both
// boxes, sunlit and in their shadows, every channel is at least the same
// pixel, less 1/255, in a reference frame whose shadows call names a second
// target, so the window's pass reads no bounce.
//
// SHADOW SIDE, at both suns (0312, 0315): the ground 0.25 m and 0.75 m out
// from the red box's shadowed -x face, at its z, which the sun does not reach,
// has its red less its green at most 8/255 above the same pixel's in that
// reference frame. The claim is the colour, not the brightness: the green
// box's lit +x face looks into that shadow and 0312 lets it light it, so only
// the red box's colour there is bounded. 8/255 is 0315's faint trace, the
// bound a 2 m probe grid can hold while the lit side keeps THE TINT.
//
// Pixels are found by projecting a world point, about the frame's eye,
// through the frame's view, with the engine's one Y flip.
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
#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 128
#define FRAMES 10
#define REDDER_BY 12
#define LATTICE 7
#define TRACE 8
// Three shapes, each drawn into four cascades, the bounce map and the view.
#define SHAPES 3

static const voe_math_float3 NEAR = { 14.0f, 0.0f, -6.0f };
static const voe_math_float3 FAR = { 12.0f, 0.0f, -14.0f };

static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = SHAPES * (VOE_RENDER_SHADOW_CASCADES + 2),
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = VOE_RENDER_SHADOW_CASCADES + 2,
	.targets = 1,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
};

// One cube `at`, scaled by `scale`, in `colour`.
static void add_a_shape(voe_ecs_world *world, voe_math_double3 at,
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
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE, .colour = colour }));
}

// The camera, a sun of `sun`, the ground and the two boxes, as the header says.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      float sun)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	// Pitched down about X by atan(8 / 14), onto the red box's foot.
	float pitch = atan2f(8.0f, 14.0f);

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 8);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 12.0, 8.0, 8.0 },
			.rotation = { -sinf(pitch * 0.5f), 0.0f, 0.0f,
				      cosf(pitch * 0.5f) },
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
		(voe_scene_light){ .colour = { 0.95097667f, 0.7098251f, 0.7098251f },
				   .intensity = sun,
				   .fill_colour = { 0.8992056f, 0.7447454f, 0.335389f },
				   .fill_intensity = 0.09f,
				   .bounces = 1 }));
	add_a_shape(world, (voe_math_double3){ 0.0, -0.05, 0.0 },
		    (voe_math_float3){ 60.0f, 0.1f, 60.0f },
		    (voe_math_float3){ 0.8f, 0.7f, 0.5f });
	add_a_shape(world, (voe_math_double3){ 12.0, 1.0, -6.0 },
		    (voe_math_float3){ 2.0f, 2.0f, 2.0f },
		    (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	add_a_shape(world, (voe_math_double3){ 6.0, 1.0, -6.0 },
		    (voe_math_float3){ 2.0f, 2.0f, 2.0f },
		    (voe_math_float3){ 0.0f, 1.0f, 0.0f });
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// One frame of the world whose shadows call updates `target`'s grid, the
// window drawn; the frame, for its view and eye.
static voe_3d_frame a_frame(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, voe_render_target target)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	bool drawing = false;

	frame.target = target;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return frame;
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	camera = (voe_render_pass_camera){ frame.view, frame.light, frame.shadow };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return frame;
}

static voe_render_picture read_window(voe_render_device *device,
				      voe_base_arena *arena)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// The pixel world point `at` lands on: about the frame's eye, through its view
// to clip, and clip +y is row 0. NULL off the picture.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_3d_frame *frame, voe_math_float3 at)
{
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(frame->view.projection, frame->view.view),
		(voe_math_float4){ at.x - (float)frame->eye.x,
				   at.y - (float)frame->eye.y,
				   at.z - (float)frame->eye.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;

	if (picture->pixels == NULL || clip.w <= 0.0f || column < 0.0f ||
	    row < 0.0f || column >= (float)picture->width ||
	    row >= (float)picture->height)
		return NULL;
	return &picture->pixels[((size_t)row * picture->width + (size_t)column) *
				4];
}

// The 8-bit red at `at`, or -1 off the picture.
static int red_at(const voe_render_picture *picture, const voe_3d_frame *frame,
		  voe_math_float3 at)
{
	const uint8_t *p = pixel_at(picture, frame, at);

	if (p == NULL)
		return -1;
	printf("(%g, %g, %g): %d %d %d\n", at.x, at.y, at.z, p[0], p[1], p[2]);
	return p[0];
}

static void the_red_box_tints_the_ground(const voe_render_picture *bounced,
					 const voe_3d_frame *frame)
{
	int near = red_at(bounced, frame, NEAR);
	int far = red_at(bounced, frame, FAR);

	VOE_TEST_CHECK(far >= 0);
	VOE_TEST_CHECK(near >= far + REDDER_BY);
}

// Every lattice point's channels in `bounced` at least `plain`'s less 1/255.
static void no_spots(const voe_render_picture *bounced,
		     const voe_render_picture *plain, const voe_3d_frame *frame)
{
	int darker = 0;

	for (int i = 0; i < LATTICE * LATTICE; i++) {
		// x across both boxes and their shadows, z before and behind.
		voe_math_float3 at = { 4.5f + 2.0f * (float)(i % LATTICE), 0.0f,
				       -12.0f + 1.5f * (float)(i / LATTICE) };
		const uint8_t *b = pixel_at(bounced, frame, at);
		const uint8_t *p = pixel_at(plain, frame, at);

		VOE_TEST_CHECK(b != NULL && p != NULL);
		if (b == NULL || p == NULL)
			continue;
		for (int c = 0; c < 3; c++)
			if ((int)b[c] < (int)p[c] - 1) {
				printf("darker at (%g, %g): %d %d %d against %d %d %d\n",
				       at.x, at.z, b[0], b[1], b[2], p[0], p[1], p[2]);
				darker++;
				break;
			}
	}
	VOE_TEST_CHECK_INT(darker, 0);
}

// The red box's shadowed foot, 0.25 m and 0.75 m out from its -x face: red
// less green in `bounced` at most `plain`'s plus TRACE.
static void the_shadow_side_stays_faint(const voe_render_picture *bounced,
					const voe_render_picture *plain,
					const voe_3d_frame *frame)
{
	static const float OUT[] = { 0.25f, 0.75f };

	for (size_t i = 0; i < sizeof(OUT) / sizeof(OUT[0]); i++) {
		voe_math_float3 at = { 11.0f - OUT[i], 0.0f, -6.0f };
		const uint8_t *b = pixel_at(bounced, frame, at);
		const uint8_t *p = pixel_at(plain, frame, at);

		VOE_TEST_CHECK(b != NULL && p != NULL);
		if (b == NULL || p == NULL)
			continue;
		printf("shadow (%g, %g): %d %d %d against %d %d %d\n", at.x,
		       at.z, b[0], b[1], b[2], p[0], p[1], p[2]);
		VOE_TEST_CHECK((int)b[0] - (int)b[1] <=
			       (int)p[0] - (int)p[1] + TRACE);
	}
}

// The scene at a sun of `sun`: ten frames bounced, one plain, the three claims.
static void at_a_sun(voe_render_device *device, const voe_3d_shapes *shapes,
		     voe_render_target other, float sun)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(arena, shapes, sun);
	voe_render_picture bounced;
	voe_render_picture plain;
	voe_3d_frame frame = { 0 };

	printf("sun %g\n", sun);
	for (int f = 0; f < FRAMES; f++)
		frame = a_frame(world, device, arena, VOE_RENDER_TARGET_WINDOW);
	bounced = read_window(device, arena);
	(void)a_frame(world, device, arena, other);
	plain = read_window(device, arena);

	the_red_box_tints_the_ground(&bounced, &frame);
	no_spots(&bounced, &plain, &frame);
	the_shadow_side_stays_faint(&bounced, &plain, &frame);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_render_target other;
	voe_render_texture shown;
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
	VOE_TEST_CHECK(voe_render_target_create(device, SIDE, SIDE, &other, &shown,
						&error));
	at_a_sun(device, &shapes, other, 1.0f);
	at_a_sun(device, &shapes, other, 3.14159265f);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
