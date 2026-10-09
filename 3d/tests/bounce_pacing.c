// That a moving eye opens at most one capture pass a frame (0397 point 2), in
// 3d's frame. The world, the frame, the settle and the passes counted are
// bounce_world.inc's.
//
// FLYING: a 1 m box at (-2.25, 0.5, 0.25), sun at bounces 1, settled with the
// eye still; then the camera moved STEP metres along +x a frame, by an intent
// the frame applies, for FRAMES frames. Each moving frame first opens empty
// passes so the device has room for exactly the cascades,
// VOE_3D_BOUNCE_MOVING_PASSES capture pass, a sun map per begun volume and the
// window's pass; its shadows call returns true on every one. BEGUN is 2: the
// 40 m ground fits a 2 m level grid, which begins the 1 m nest alone. Before
// 0397 the 1 m nest's slices, queued as the eye walks, opened up to four
// passes and that frame failed.
//
// STILL AGAIN: the eye left where it stopped, the probing frames settle within
// the harness's BOUND.
//
// IT NEEDS A GRAPHICS CARD WITH shaderOutputLayer AND SKIPS WITH A REASON
// WITHOUT ONE, as bounce_scene.c does.
#include <scene/light_system.h>

#include "bounce_world.inc"

#define STEP 0.15
#define FRAMES 120
#define BEGUN 2
// Leaves room for the cascades, one capture pass, a sun map per begun volume
// and the window's pass.
#define MOVING_DUMMIES                                                    \
	(PASSES - (VOE_RENDER_SHADOW_CASCADES +                            \
		   (int)VOE_3D_BOUNCE_MOVING_PASSES + BEGUN + 1))

// The camera moved STEP along +x by an intent the next frame applies.
static void step_the_eye(scene *s)
{
	voe_scene_transform camera = *voe_scene_transform_get(s->world, s->camera);

	camera.position.x += STEP;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ s->camera, camera }));
}

// One moving frame on the room of the header. Returns whether its shadows
// call was true.
static bool a_moving_frame_fits(scene *s)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(s->scratch);
	bool drawing;
	voe_3d_frame frame = begin_a_frame(s, &drawing);
	voe_render_pass_camera camera;
	bool fits = false;

	if (drawing) {
		for (int i = 0; i < MOVING_DUMMIES; i++) {
			VOE_TEST_CHECK(voe_render_pass_begin(
				s->device, VOE_RENDER_TARGET_WINDOW, NULL));
			voe_render_pass_end(s->device);
		}
		fits = voe_3d_draw_system_shadows(s->world, s->device, &frame);
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
	return fits;
}

// The case of the header.
static void flying_captures_one_pass_a_frame(scene *s,
					      const voe_3d_shapes *shapes)
{
	int refused = 0;

	a_world(s, shapes, 1, (voe_math_double3){ -2.25, 0.5, 0.25 },
		(voe_math_float3){ 1.0f, 1.0f, 1.0f },
		(voe_math_float3){ 0.5f, 0.1f, 0.6f });
	VOE_TEST_CHECK(settles(s));
	for (int f = 0; f < FRAMES; f++) {
		step_the_eye(s);
		if (!a_moving_frame_fits(s))
			refused++;
	}
	if (refused > 0)
		printf("%d of %d moving frames wanted more room\n", refused,
		       FRAMES);
	VOE_TEST_CHECK(refused == 0);
	VOE_TEST_CHECK(settles(s));
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
		flying_captures_one_pass_a_frame(&s, &shapes);
	}
	if (s.device != NULL)
		voe_render_device_destroy(s.device);
	voe_base_arena_destroy(s.scratch);
	voe_base_arena_destroy(s.keep);
	return voe_test_result();
}
