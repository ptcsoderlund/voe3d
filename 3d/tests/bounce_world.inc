// The probe bounce's test world and the frame that draws it, compiled into
// each program that includes it: bounce_scene.c, and the bounce tests after
// it. It holds the world, its reference, one frame as the editor and the game
// draw it, the settle and the window read; the cases stay in the programs.
// It brings its own includes, so a program needs only this line for them.
//
// THE WORLD. Grey ground (a flat box 40 m wide, top at y 0, 0.5 grey, so the
// grid stays at 2 m as in the tank game's); one box the caller places, sizes
// and colours; a sun of 1 shining down at 45 degrees along -x, `bounces`
// bounces, strength 1, no fill, casting, as are both shapes (0324). The eye
// stands 5 m up and 8 m back (+z), looking down at the origin: a small
// thing's colour is a near-camera detail (0387, 0390). The reference is the
// same world at bounces 0: its shadows call begins no bounce, so its window
// reads none. Each world keeps a previous table and the shape changes table,
// and every frame first remembers transforms, then runs the transform system,
// as the editor's step does; a move is an intent that frame applies.
//
// THE FRAME: voe_3d_draw_system_frame, _point_lights, _light_blockers,
// _shadows with `frame.target` the window, the window's pass, _run, frame end.
// Nothing hand-fitted.
//
// SETTLED, COUNTED AS bounce.c COUNTS: by `passes`. A full frame has room for
// the cascades, VOE_RENDER_BOUNCE_CAPTURE_PASSES capture passes, a sun map per
// begun volume (0389: a 2 m level grid begins the 1 m nest too; the device has
// room for VOE_RENDER_BOUNCE_VOLUMES) and the window's. A probing frame first
// opens DUMMIES empty passes, so a shadows call that would open a capture pass
// or a sun map returns false there (render says so on a line; that noise is
// the measurement), true when nothing is queued or to be relit. Full and
// probing frames alternate until a probing one is true, within BOUND pairs;
// then FADE full frames more, as a frame that only fades opens no pass.
//
// Pixels are a world point projected about the frame's eye through its view,
// with the engine's one Y flip. The device is the caller's, made with
// CAPACITIES at SIDE a side; everything here allocates from the scene's two
// arenas, `keep` for the worlds and pictures, `scratch` rewound each frame.
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
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 128
#define BOUND 320
// Places a new probe picture takes to full weight (0389 point 4), render's
// VOE_RENDER_BOUNCE_FADE, which render keeps to itself.
#define FADE 16
#define TINT 3
#define SHADOW 4
#define EVEN 2
// The cascades, the capture passes, a bounce shadow pass for each volume a
// target holds and the window's.
#define PASSES                                                          \
	(VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES + \
	 VOE_RENDER_BOUNCE_VOLUMES + 1)
// Leaves the cascades' passes and not one more.
#define DUMMIES (PASSES - VOE_RENDER_SHADOW_CASCADES)

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

// The world of the header into `s->world`, with a sun of `bounces` and the box
// at `centre`, `size` a side per axis, in `colour`, its entity `s->box`. The
// world lives in `s->keep`, freed with that arena.
static void a_world(scene *s, const voe_3d_shapes *shapes, uint32_t bounces,
		    voe_math_double3 centre, voe_math_float3 size,
		    voe_math_float3 colour)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	float pitch = atan2f(5.0f, 8.0f);

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
	voe_3d_shape_changes_register(s->world, 8);
	voe_3d_model_register(s->world, 8);
	voe_scene_light_blocker_register(s->world, 2);

	VOE_TEST_CHECK(voe_ecs_entity_create(s->world, &s->camera));
	VOE_TEST_CHECK(voe_scene_transform_add(
		s->world, s->camera,
		(voe_scene_transform){
			.position = { 0.0, 5.0, 8.0 },
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
			  (voe_math_float3){ 40.0f, 0.1f, 40.0f },
			  (voe_math_float3){ 0.5f, 0.5f, 0.5f });
	s->box = add_a_shape(s->world, centre, size, colour);
	voe_3d_shape_system_run(s->world, shapes);
}

// The reference of the bounced scene `s`: the same world at bounces 0, its box
// where, as large and as coloured as `s`'s now is, on `s`'s device and arenas.
// Returns the scene; its world lives in `s->keep`, freed with that arena.
static scene a_reference_world(const scene *s, const voe_3d_shapes *shapes)
{
	scene plain = *s;
	const voe_scene_transform *box = voe_scene_transform_get(s->world, s->box);
	const voe_3d_shape *shape = voe_3d_shape_get(s->world, s->box);

	VOE_TEST_CHECK(box != NULL && shape != NULL);
	if (box == NULL || shape == NULL)
		return plain;
	a_world(&plain, shapes, 0, box->position, box->scale, shape->colour);
	return plain;
}

// The frame's step and then its view and lights, as the editor takes them
// each frame: every transform remembered, then the transforms placed since
// applied, so a place is a move between lag 1 and lag 0. Returns the frame,
// its lists in `s->scratch`; `*drawing` whether the device began one.
static voe_3d_frame begin_a_frame(scene *s, bool *drawing)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame;

	voe_scene_transform_remember(s->world);
	voe_scene_transform_system_run(s->world);
	frame = voe_3d_draw_system_frame(s->world, size, 0.0f);
	frame.target = VOE_RENDER_TARGET_WINDOW;
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(s->world, &frame,
						       s->scratch));
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(s->world, &frame,
							 s->scratch));
	*drawing = false;
	VOE_TEST_CHECK(voe_render_frame_begin(s->device, size, drawing));
	VOE_TEST_CHECK(*drawing);
	return frame;
}

// One frame as the editor and the game draw it: the shadows call, the
// window's pass, frame end. Returns the frame, for its view and eye; its lists
// are gone with the scratch rewind, the view and eye are values.
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
						   .points = frame.points,
						   .blockers = frame.blockers };
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
// left. Returns whether it would have opened a capture pass or a sun map.
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

// Full and probing frames until a probing one wants no capture, then FADE
// full frames, since a frame that only fades opens no pass and a read could
// otherwise land mid-fade. Returns whether that came within BOUND pairs.
static bool settles(scene *s)
{
	for (int pair = 0; pair < BOUND; pair++) {
		(void)a_full_frame(s);
		if (!a_capture_was_wanted(s)) {
			printf("settled after %d pairs\n", pair + 1);
			for (int f = 0; f < FADE; f++)
				(void)a_full_frame(s);
			return true;
		}
	}
	return false;
}

// The window as last drawn. Returns the picture, its pixels in `s->keep`,
// freed with that arena.
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
// to clip, and clip +y is row 0. Returns its four bytes inside `picture`; a
// black pixel's worth of static zeros off the picture, with the check failed.
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
