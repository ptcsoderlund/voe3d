// That the tint a small red box gives the ground beside it outlives a whole
// relight (work order 083). The world, its reference, the frame, the settle and
// the window read are bounce_world.inc's, built as bounce_scene.c builds them:
// the box 2 m a side, strongly red, at (0.5, 1, 0.5), the sun on its +x face,
// the eye 5 m up and 8 m back, so the patch lies in the 1 m nest.
//
// A WHOLE RELIGHT AFTER A MOVE KEEPS THE TINT: settled, the box moved 1 m along
// +x, the probing frame that applies the move wanting a capture as in
// bounce_scene.c's SETTLING (it once left the nests unbegun that step, so they
// kept the old pictures), then settled; the ground 0.75 m out from its sunlit
// face read; then the bounce strength set to 2 and settled, back to 1 and
// settled, each a change of
// the lights that relights every probe of every volume (0389 point 5). The
// patch is then within 1/255 of the read before in every channel, and its red
// less green above the reference world's (bounces 0, the box where it now
// stands) by at least TINT/255. The three patches are printed on failure.
//
// Follows 0389 (the nests and what a lights change relights), 0390 (the read
// beside a box) and 0393 (the tint back after a whole relight is 083's).
//
// IT NEEDS A GRAPHICS CARD WITH shaderOutputLayer AND SKIPS WITH A REASON
// WITHOUT ONE, as bounce_scene.c does.
#include <scene/light_system.h>

#include <stdlib.h>

#include "bounce_world.inc"

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

// The patch 0.75 m out from the box's sunlit face, as last drawn, into `rgb`.
static void the_patch(scene *s, uint8_t rgb[3])
{
	const voe_scene_transform *box = voe_scene_transform_get(s->world, s->box);
	voe_math_float3 patch = { (float)box->position.x + 1.75f, 0.0f,
				  (float)box->position.z };
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);
	const uint8_t *p = pixel_at(&picture, &frame, patch);

	for (int c = 0; c < 3; c++)
		rgb[c] = p[c];
}

// The case of the header.
static void a_whole_relight_after_a_move_keeps_the_tint(scene *s,
							 const voe_3d_shapes *shapes)
{
	voe_scene_transform moved;
	scene plain;
	uint8_t before[3];
	uint8_t after[3];
	uint8_t reference[3];
	bool kept = true;

	a_world(s, shapes, 1, (voe_math_double3){ 0.5, 1.0, 0.5 },
		(voe_math_float3){ 2.0f, 2.0f, 2.0f },
		(voe_math_float3){ 1.0f, 0.02f, 0.02f });
	VOE_TEST_CHECK(settles(s));
	moved = *voe_scene_transform_get(s->world, s->box);
	moved.position.x += 1.0;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ s->box, moved }));
	VOE_TEST_CHECK(a_capture_was_wanted(s));
	VOE_TEST_CHECK(settles(s));
	the_patch(s, before);
	bounce_strength(s, 2.0f);
	VOE_TEST_CHECK(settles(s));
	bounce_strength(s, 1.0f);
	VOE_TEST_CHECK(settles(s));
	the_patch(s, after);
	plain = a_reference_world(s, shapes);
	(void)a_full_frame(&plain);
	the_patch(&plain, reference);

	for (int c = 0; c < 3; c++)
		kept = kept && abs((int)after[c] - (int)before[c]) <= 1;
	kept = kept && ((int)after[0] - (int)after[1]) -
				       ((int)reference[0] - (int)reference[1]) >=
			       TINT;
	if (!kept)
		printf("patch moved %d %d %d, relit %d %d %d, reference %d %d %d\n",
		       before[0], before[1], before[2], after[0], after[1],
		       after[2], reference[0], reference[1], reference[2]);
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)after[c] - (int)before[c]) <= 1);
	VOE_TEST_CHECK(((int)after[0] - (int)after[1]) -
			       ((int)reference[0] - (int)reference[1]) >=
		       TINT);
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
		a_whole_relight_after_a_move_keeps_the_tint(&s, &shapes);
	}
	if (s.device != NULL)
		voe_render_device_destroy(s.device);
	voe_base_arena_destroy(s.scratch);
	voe_base_arena_destroy(s.keep);
	return voe_test_result();
}
