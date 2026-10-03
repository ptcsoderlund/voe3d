// The probe bounce as the editor and the game draw it (feature 051, How to test
// steps 2, 3 and 8), through 3d's own calls a frame: voe_3d_draw_system_frame,
// _point_lights, _shadows with `frame.target` the window, the window's pass,
// _run, frame end; then the window read. Nothing hand-fitted: the grid, the
// casters, the stale spheres and the relight are 3d's and render's.
//
// THE WORLD. Grey ground (a flat box 60 m wide, top at y 0, 0.5 grey); a box
// 2 m a side, strongly red, standing at (0.5, 1, 0.5), off the probe lattice's
// odd metres so no probe sits on its faces; a sun of 1 shining down at 45
// degrees along -x onto the box's +x face, bounces 1 and strength 1, no fill,
// casting, as are both shapes (0324). The eye stands 8 m up and 14 m back
// (+z), looking down at the origin. The same world at bounces 0 is the
// reference: its shadows call begins no bounce, so its window reads none.
//
// SETTLED, AND COUNTED AS bounce.c COUNTS: by `passes`. A full frame has room
// for the cascades, VOE_RENDER_BOUNCE_CAPTURE_PASSES capture passes, the
// relight's sun map (0329) and the window's. A probing frame first opens
// DUMMIES empty passes, so the cascades spend the rest: a shadows call that
// would open a capture pass or the sun map returns false there (render says so
// on a line; that noise is the measurement), and true when nothing is queued
// and nothing is to be relit. Frames alternate full and probing until a probing one
// is true, within BOUND pairs or the test fails: the whole grid is 6912 probes
// at 64 a full frame.
//
// THE LIT SIDE (step 2): the ground 0.25 m out from the box's sunlit +x face
// has its red less its green above the reference's by at least TINT/255, and
// 3 m out by less than half that: the box's colour reaches the ground it faces,
// and fades.
//
// THE SHADOW (step 3, 0312): the ground 0.25 m out from the shadowed -x face,
// in the box's shadow at its foot, within SHADOW/255 of the reference in every
// channel: the lit face does not light its own shadow. It holds because of
// 0327: a probe's six-axis irradiance gives flat ground nothing from below its
// own plane, where L1 SH lit the foot from the sunlit ground ring.
//
// EVEN GROUND (step 2, 0326's reasoning): five points of open ground along
// x = -6, 2 m apart in z, within EVEN/255 of each other in every channel; and
// again after the sun is turned 5 degrees higher, each point's green having
// risen with it, so the evenness is not an unlit floor's.
//
// SETTLING (step 8): with nothing changed the next probing frame opens no
// capture pass; the box moved 1 m along x by its transform, with a previous
// table remembered as a stepping game does, makes the next one open one (its
// stale spheres queue probes), and within BOUND it settles again.
//
// TURN (bug 01, 0328, 0329): settled, the camera turned 90 degrees about Y in
// place for TURNED probing frames, each opening no capture pass, then turned
// back: the lit-side, shadow-foot and five open-ground pixels each within 1/255
// of their values before the turn. Turning moves no probe and lights nothing.
//
// LOOKING AWAY (0328, 0329): settled, the camera turned 180 degrees in place;
// facing away, the sun's bounce strength set to 2 and settled, set back to 1
// and settled; turned back, the same seven pixels each within 1/255 of their
// values before the turn. A relight made while the room is behind the camera
// shadows the sun by the volume's own map, so it lights the room as one made
// facing it does; with the view's cascades it lit the room unshadowed.
//
// Pixels are found by projecting a world point, about the frame's eye, through
// the frame's view, with the engine's one Y flip.
//
// IT NEEDS A GRAPHICS CARD WITH shaderOutputLayer AND SKIPS WITH A REASON
// WITHOUT ONE, as 3d/tests/shadows.c does: without it nothing is captured.
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
#include <math/quat.h>
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
#include <stdlib.h>

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 128
#define BOUND 160
#define TINT 3
#define SHADOW 4
#define EVEN 2
#define OPEN 5
#define TURNED 3
// The pixels TURN compares: the lit side, the shadow's foot and the open ground.
#define LOOKED (2 + OPEN)
// The cascades, the capture passes, the bounce shadow pass and the window's.
#define PASSES \
	(VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES + 2)
// Leaves the cascades' passes and not one more.
#define DUMMIES (VOE_RENDER_BOUNCE_CAPTURE_PASSES + 2)

static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = 2 * PASSES + 8,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = PASSES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
};

// One world, the device it is drawn on and the arenas: `scratch` rewound each
// frame, `keep` for the pictures read back.
typedef struct {
	voe_ecs_world *world;
	voe_ecs_entity camera;
	voe_ecs_entity box;
	voe_ecs_entity sun;
	voe_render_device *device;
	voe_base_arena *scratch;
	voe_base_arena *keep;
} scene;

// One cube `at`, scaled by `scale`, in `colour`, casting; the entity.
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

// The sun's rotation shining along -x, `elevation` radians down.
static voe_math_quat sun_at(float elevation)
{
	return voe_scene_light_facing(
		(voe_math_float3){ -cosf(elevation), -sinf(elevation), 0.0f });
}

// The world of the header with a sun of `bounces`, remembered once.
static void a_world(scene *s, const voe_3d_shapes *shapes, uint32_t bounces)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	float pitch = atan2f(8.0f, 14.0f);

	s->world = voe_ecs_world_new(s->keep, limits);
	voe_scene_transform_register(s->world, 8);
	voe_scene_transform_previous_register(s->world, 8);
	voe_scene_camera_register(s->world, 2);
	voe_scene_light_register(s->world, 2);
	voe_scene_point_light_register(s->world, 2);
	voe_3d_mesh_register(s->world, 8);
	voe_3d_material_register(s->world, 8);
	voe_3d_panel_register(s->world, 8);
	voe_3d_shape_register(s->world, 8);
	voe_3d_model_register(s->world, 8);

	VOE_TEST_CHECK(voe_ecs_entity_create(s->world, &s->camera));
	VOE_TEST_CHECK(voe_scene_transform_add(
		s->world, s->camera,
		(voe_scene_transform){
			.position = { 0.0, 8.0, 14.0 },
			.rotation = { -sinf(pitch * 0.5f), 0.0f, 0.0f,
				      cosf(pitch * 0.5f) },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		s->world, s->camera,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(s->world, &s->sun));
	VOE_TEST_CHECK(voe_scene_transform_add(
		s->world, s->sun,
		(voe_scene_transform){ .rotation = sun_at(0.78539816f),
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		s->world, s->sun,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.0f,
				   .bounces = bounces,
				   .bounce_strength = 1.0f,
				   .cast_shadows = true }));
	(void)add_a_shape(s->world, (voe_math_double3){ 0.0, -0.05, 0.0 },
			  (voe_math_float3){ 60.0f, 0.1f, 60.0f },
			  (voe_math_float3){ 0.5f, 0.5f, 0.5f });
	s->box = add_a_shape(s->world, (voe_math_double3){ 0.5, 1.0, 0.5 },
			     (voe_math_float3){ 2.0f, 2.0f, 2.0f },
			     (voe_math_float3){ 1.0f, 0.02f, 0.02f });
	voe_3d_shape_system_run(s->world, shapes);
	voe_scene_transform_remember(s->world);
}

// The frame's view and lights, as the loop takes them each frame.
static voe_3d_frame begin_a_frame(scene *s, bool *drawing)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(s->world, size, 0.0f);

	frame.target = VOE_RENDER_TARGET_WINDOW;
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(s->world, &frame,
						       s->scratch));
	*drawing = false;
	VOE_TEST_CHECK(voe_render_frame_begin(s->device, size, drawing));
	VOE_TEST_CHECK(*drawing);
	return frame;
}

// One frame as the editor and the game draw it; the frame, for its view.
static voe_3d_frame a_full_frame(scene *s)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(s->scratch);
	bool drawing;
	voe_3d_frame frame = begin_a_frame(s, &drawing);
	voe_render_pass_camera camera;

	if (drawing) {
		VOE_TEST_CHECK(voe_3d_draw_system_shadows(s->world, s->device,
							  &frame));
		camera = (voe_render_pass_camera){ .view = frame.view,
						   .light = frame.light,
						   .shadow = frame.shadow,
						   .points = frame.points };
		VOE_TEST_CHECK(voe_render_pass_begin(
			s->device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(s->world, s->device, s->scratch, frame);
		voe_render_pass_end(s->device);
		VOE_TEST_CHECK(voe_render_frame_end(s->device));
	}
	voe_base_arena_rewind(s->scratch, mark);
	return frame;
}

// A probing frame: DUMMIES empty passes, then the shadows call on what is
// left. Whether it would have opened a capture pass.
static bool a_capture_was_wanted(scene *s)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(s->scratch);
	bool drawing;
	voe_3d_frame frame = begin_a_frame(s, &drawing);
	bool wanted = false;

	if (drawing) {
		for (int i = 0; i < DUMMIES; i++) {
			VOE_TEST_CHECK(voe_render_pass_begin(
				s->device, VOE_RENDER_TARGET_WINDOW, NULL));
			voe_render_pass_end(s->device);
		}
		wanted = !voe_3d_draw_system_shadows(s->world, s->device, &frame);
		VOE_TEST_CHECK(voe_render_frame_end(s->device));
	}
	voe_base_arena_rewind(s->scratch, mark);
	return wanted;
}

// Full and probing frames until a probing one wants no capture; whether that
// came within BOUND.
static bool settles(scene *s)
{
	for (int pair = 0; pair < BOUND; pair++) {
		(void)a_full_frame(s);
		if (!a_capture_was_wanted(s)) {
			printf("settled after %d pairs\n", pair + 1);
			return true;
		}
	}
	return false;
}

static voe_render_picture read_window(scene *s)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_target_read(s->device, VOE_RENDER_TARGET_WINDOW,
					      s->keep, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// The pixel world point `at` lands on: about the frame's eye, through its view
// to clip, and clip +y is row 0. A black pixel's worth of zeros off the picture,
// with the check failed.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_3d_frame *frame, voe_math_float3 at)
{
	static const uint8_t NONE[4] = { 0 };
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(frame->view.projection, frame->view.view),
		(voe_math_float4){ at.x - (float)frame->eye.x,
				   at.y - (float)frame->eye.y,
				   at.z - (float)frame->eye.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;
	bool off = picture->pixels == NULL || clip.w <= 0.0f || column < 0.0f ||
		   row < 0.0f || column >= (float)picture->width ||
		   row >= (float)picture->height;

	VOE_TEST_CHECK(!off);
	if (off)
		return NONE;
	return &picture->pixels[((size_t)row * picture->width + (size_t)column) *
				4];
}

// Red less green at `at` in `bounced` above the same in `plain`.
static int redder_by(const voe_render_picture *bounced,
		     const voe_render_picture *plain, const voe_3d_frame *frame,
		     voe_math_float3 at)
{
	const uint8_t *b = pixel_at(bounced, frame, at);
	const uint8_t *p = pixel_at(plain, frame, at);

	printf("(%g, %g): %d %d %d against %d %d %d\n", at.x, at.z, b[0], b[1],
	       b[2], p[0], p[1], p[2]);
	return ((int)b[0] - (int)b[1]) - ((int)p[0] - (int)p[1]);
}

// The lit side's tint and its fade; the shadow at the foot unchanged.
static void the_box_colours_its_lit_side_only(const voe_render_picture *bounced,
					      const voe_render_picture *plain,
					      const voe_3d_frame *frame)
{
	int near = redder_by(bounced, plain, frame,
			     (voe_math_float3){ 1.75f, 0.0f, 0.5f });
	int far = redder_by(bounced, plain, frame,
			    (voe_math_float3){ 4.5f, 0.0f, 0.5f });
	voe_math_float3 foot = { -0.75f, 0.0f, 0.5f };
	const uint8_t *b = pixel_at(bounced, frame, foot);
	const uint8_t *p = pixel_at(plain, frame, foot);

	VOE_TEST_CHECK(near >= TINT);
	VOE_TEST_CHECK(2 * far < near);
	printf("foot: %d %d %d against %d %d %d\n", b[0], b[1], b[2], p[0],
	       p[1], p[2]);
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)b[c] - (int)p[c]) <= SHADOW);
}

// The five open points' pixels into `pixels`, checked even.
static void open_ground_is_even(const voe_render_picture *picture,
				const voe_3d_frame *frame,
				uint8_t pixels[OPEN][3])
{
	for (int i = 0; i < OPEN; i++) {
		const uint8_t *p = pixel_at(
			picture, frame,
			(voe_math_float3){ -6.0f, 0.0f, -6.0f + 2.0f * (float)i });

		printf("open %d: %d %d %d\n", i, p[0], p[1], p[2]);
		for (int c = 0; c < 3; c++)
			pixels[i][c] = p[c];
	}
	for (int i = 1; i < OPEN; i++)
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)pixels[i][c] - (int)pixels[0][c]) <=
				       EVEN);
}

// `entity` given `transform` by an intent and the transform system's run.
static void place(scene *s, voe_ecs_entity entity, voe_scene_transform transform)
{
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ entity, transform }));
	voe_scene_transform_system_run(s->world);
}

// The LOOKED pixels of `picture` into `pixels`: lit side, foot, open ground.
static void the_pixels_looked_at(const voe_render_picture *picture,
				 const voe_3d_frame *frame,
				 uint8_t pixels[LOOKED][3])
{
	voe_math_float3 at[LOOKED] = { { 1.75f, 0.0f, 0.5f },
				       { -0.75f, 0.0f, 0.5f } };

	VOE_TEST_CHECK(picture != NULL && frame != NULL);
	for (int i = 0; i < OPEN; i++)
		at[2 + i] = (voe_math_float3){ -6.0f, 0.0f, -6.0f + 2.0f * (float)i };
	for (int i = 0; i < LOOKED; i++) {
		const uint8_t *p = pixel_at(picture, frame, at[i]);

		for (int c = 0; c < 3; c++)
			pixels[i][c] = p[c];
	}
	VOE_TEST_CHECK(LOOKED == 2 + OPEN);
}

// TURN: a quarter turn about Y in place and back opens no capture pass and
// leaves every looked-at pixel within 1/255.
static void turning_moves_nothing(scene *s)
{
	voe_scene_transform pose = *voe_scene_transform_get(s->world, s->camera);
	voe_scene_transform turned = pose;
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);
	uint8_t before[LOOKED][3];
	uint8_t after[LOOKED][3];

	the_pixels_looked_at(&picture, &frame, before);
	turned.rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle((voe_math_float3){ 0.0f, 1.0f, 0.0f },
					      1.5707963f),
		pose.rotation);
	place(s, s->camera, turned);
	for (int f = 0; f < TURNED; f++)
		VOE_TEST_CHECK(!a_capture_was_wanted(s));
	place(s, s->camera, pose);
	frame = a_full_frame(s);
	picture = read_window(s);
	the_pixels_looked_at(&picture, &frame, after);
	for (int i = 0; i < LOOKED; i++) {
		printf("turned %d: %d %d %d against %d %d %d\n", i, after[i][0],
		       after[i][1], after[i][2], before[i][0], before[i][1],
		       before[i][2]);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)after[i][c] - (int)before[i][c]) <= 1);
	}
}

// The sun's bounce strength set to `strength` by an intent and the light
// system's run.
static void bounce_strength(scene *s, float strength)
{
	voe_scene_light light = *voe_scene_light_get(s->world, s->sun);

	light.bounce_strength = strength;
	VOE_TEST_CHECK(voe_scene_light_submit(
		s->world, (voe_scene_light_intent){ s->sun, light }));
	voe_scene_light_system_run(s->world);
}

// LOOKING AWAY: relit twice facing away, then turned back, every looked-at
// pixel within 1/255 of before the turn.
static void looking_away_relights_the_same(scene *s)
{
	voe_scene_transform pose = *voe_scene_transform_get(s->world, s->camera);
	voe_scene_transform turned = pose;
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);
	uint8_t before[LOOKED][3];
	uint8_t after[LOOKED][3];

	the_pixels_looked_at(&picture, &frame, before);
	turned.rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle((voe_math_float3){ 0.0f, 1.0f, 0.0f },
					      3.14159265f),
		pose.rotation);
	place(s, s->camera, turned);
	VOE_TEST_CHECK(settles(s));
	bounce_strength(s, 2.0f);
	VOE_TEST_CHECK(settles(s));
	bounce_strength(s, 1.0f);
	VOE_TEST_CHECK(settles(s));
	place(s, s->camera, pose);
	frame = a_full_frame(s);
	picture = read_window(s);
	the_pixels_looked_at(&picture, &frame, after);
	for (int i = 0; i < LOOKED; i++) {
		printf("looked away %d: %d %d %d against %d %d %d\n", i,
		       after[i][0], after[i][1], after[i][2], before[i][0],
		       before[i][1], before[i][2]);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)after[i][c] - (int)before[i][c]) <= 1);
	}
}

// The claims, on a device with shaderOutputLayer.
static void the_bounce(scene *s, const voe_3d_shapes *shapes)
{
	scene plain = *s;
	voe_render_picture bounced;
	voe_render_picture reference;
	voe_render_picture turned;
	voe_scene_transform moved;
	voe_3d_frame frame;
	uint8_t before[OPEN][3];
	uint8_t after[OPEN][3];

	a_world(s, shapes, 1);
	a_world(&plain, shapes, 0);
	VOE_TEST_CHECK(settles(s));
	frame = a_full_frame(s);
	bounced = read_window(s);
	(void)a_full_frame(&plain);
	(void)a_full_frame(&plain);
	reference = read_window(&plain);
	the_box_colours_its_lit_side_only(&bounced, &reference, &frame);
	open_ground_is_even(&bounced, &frame, before);

	moved = *voe_scene_transform_get(s->world, s->sun);
	moved.rotation = sun_at(0.87266463f);
	place(s, s->sun, moved);
	for (int f = 0; f < 3; f++)
		frame = a_full_frame(s);
	turned = read_window(s);
	open_ground_is_even(&turned, &frame, after);
	for (int i = 0; i < OPEN; i++)
		VOE_TEST_CHECK(after[i][1] > before[i][1]);

	VOE_TEST_CHECK(!a_capture_was_wanted(s));
	voe_scene_transform_remember(s->world);
	moved = *voe_scene_transform_get(s->world, s->box);
	moved.position.x += 1.0;
	place(s, s->box, moved);
	VOE_TEST_CHECK(a_capture_was_wanted(s));
	voe_scene_transform_remember(s->world);
	VOE_TEST_CHECK(settles(s));
	turning_moves_nothing(s);
	looking_away_relights_the_same(s);
}

int main(void)
{
	scene s = { .keep = voe_base_arena_new(SCRATCH),
		    .scratch = voe_base_arena_new(SCRATCH) };
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_3d_shapes shapes;

	s.device = voe_render_device_new_headless(s.keep, size, CAPACITIES, &error);
	if (s.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(s.device != NULL);
	} else if (!voe_render_point_shadows_ready(s.device)) {
		printf("skip: no shaderOutputLayer, so nothing is captured\n");
	} else {
		VOE_TEST_CHECK(voe_3d_shapes_upload(s.device, &shapes, &error));
		the_bounce(&s, &shapes);
	}
	if (s.device != NULL)
		voe_render_device_destroy(s.device);
	voe_base_arena_destroy(s.scratch);
	voe_base_arena_destroy(s.keep);
	return voe_test_result();
}
