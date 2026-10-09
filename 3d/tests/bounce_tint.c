// That a small box tints the ground beside it, in 3d's frame (work order 083).
// The world, its reference, the frame, the settle and the window read are
// bounce_world.inc's. Each case runs on a device of its own, so no case reads
// probe pictures another case's world left. The reference is the same world at
// bounces 0, the box where it now stands: without its bounce, not without it,
// so its shadow is in both. Each case prints what it read on failure.
//
// THE PURPLE CUBE (How to test step 1): 1 m, linear (0.5, 0.1, 0.6), at
// (-2.25, 0.5, 0.25), off the 1 m nest's half metres and the 2 m grid's odd
// ones, its sunlit +x face toward the eye at x 0; the sun LOW, bounces 1,
// strength 2, settled.
//
// A SMALL BOX TINTS THE GROUND BESIDE IT (steps 2, 3 and 6): the ground 0.25 m
// out from the lit face redder and bluer than the reference by at least
// TINT/255, its green rising less than half as much; 3 m out by less than half
// TINT/255; the foot of the shaded face within SHADOW/255 in every channel. At
// strength 5 the 0.25 m red and blue each at least 1/255 more; back at 2,
// within 1/255 of the first read. Not "no greener": the cube's own 0.1 green
// bounces too, 8/255 against red's 32.
//
// A SLAB REDDENS THE BOX FACE (step 3): at strength 5, a red slab lying on the
// floor, 2 m out along the face's normal, 1 m across, 0.2 m high, its centre
// 1.5 m out, under the sun's line to the face, settled: the middle of the lit
// face at least TINT/255 redder than before. It is added under the floor and
// raised by an intent, as a frame here remembers before it draws, so a shape
// added between frames is never new to it. Lying across the face, its dark
// side hid as much lit floor as its top added: red fell 2/255.
//
// A MOVED BOX LEAVES NO RING (steps 4 and 5): the cube moved MOVE metres along
// its lit face's normal and settled; the ground 0.25 m out from the new face
// tinted as above; the old 0.25 m point and eight points on a 2 m circle about
// the old place within EVEN/255 of the reference. MOVE is 3, not the card's 1:
// moved 1 m, the cube stands on the old 0.25 m point and its new tint reaches
// the circle, so nothing of the old place could be read.
//
// A RECOLOURED BOX TINTS ITS NEW COLOUR (0389 point 7): the cube recoloured a
// green with no red through voe_3d_shape_submit, the shape run that marks it,
// a frame, the run that clears the mark, settled: the 0.25 m ground greener
// than the reference by at least TINT/255 and redder by less than half
// TINT/255, so any red is the purple's, kept.
//
// A WHOLE RELIGHT AFTER A MOVE KEEPS THE TINT: the box 2 m a side, strongly
// red, at (0.5, 1, 0.5), the sun at 45 degrees, settled, moved 1 m along +x,
// the probing frame that applies the move wanting a capture as in
// bounce_scene.c's SETTLING (it once left the nests unbegun that step, so they
// kept the old pictures), then settled; the ground 0.75 m out from its sunlit
// face read; then the bounce strength set to 2 and settled, back to 1 and
// settled, each a change of the lights that relights every probe of every
// volume (0389 point 5). The patch is then within 1/255 of the read before in
// every channel, and its red less green above the reference's by at least
// TINT/255.
//
// Follows 0387 and 0389 (the nests and what a lights change relights), 0390
// (the read beside a box) and 0393 (the tint back after a whole relight).
//
// IT NEEDS A GRAPHICS CARD WITH shaderOutputLayer AND SKIPS WITH A REASON
// WITHOUT ONE, as bounce_scene.c does.
#include <scene/light_system.h>

#include <stdlib.h>

#include "bounce_world.inc"

// The sun's elevation, radians: 30 degrees, low.
#define LOW 0.52359878f
#define MOVE 3.0
#define RING 8

static const voe_math_float3 PURPLE = { 0.5f, 0.1f, 0.6f };
static const voe_math_float3 GREEN = { 0.0f, 0.6f, 0.0f };
static const voe_math_float3 RED = { 1.0f, 0.02f, 0.02f };

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

// The sun turned LOW by an intent the next frame applies.
static void low_sun(scene *s)
{
	voe_scene_transform sun = *voe_scene_transform_get(s->world, s->sun);

	sun.rotation = sun_at(LOW);
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ s->sun, sun }));
}

// The purple cube's world of the header, settled.
static void a_purple_world(scene *s, const voe_3d_shapes *shapes)
{
	a_world(s, shapes, 1, (voe_math_double3){ -2.25, 0.5, 0.25 },
		(voe_math_float3){ 1.0f, 1.0f, 1.0f }, PURPLE);
	low_sun(s);
	bounce_strength(s, 2.0f);
	VOE_TEST_CHECK(settles(s));
}

// The reference of `s` with the sun LOW, one frame drawn. Returns the scene.
static scene a_low_reference(scene *s, const voe_3d_shapes *shapes)
{
	scene plain = a_reference_world(s, shapes);

	low_sun(&plain);
	(void)a_full_frame(&plain);
	return plain;
}

// The ground `out` metres beyond the box's sunlit +x face, level with its
// centre; less than 0, beyond its shaded -x face.
static voe_math_float3 beside(const scene *s, float out)
{
	const voe_scene_transform *box = voe_scene_transform_get(s->world, s->box);
	float half = box->scale.x * 0.5f;

	return (voe_math_float3){
		(float)box->position.x + (out < 0.0f ? out - half : out + half),
		0.0f, (float)box->position.z };
}

// The window as `s` now draws it at each of the `count` points `at`, into `rgb`.
static void looks(scene *s, int count, const voe_math_float3 *at,
		  uint8_t (*rgb)[3])
{
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);

	for (int i = 0; i < count; i++) {
		const uint8_t *p = pixel_at(&picture, &frame, at[i]);

		for (int c = 0; c < 3; c++)
			rgb[i][c] = p[c];
	}
}

// Channel `c` of `a` above the same of `b`.
static int rise(const uint8_t *a, const uint8_t *b, int c)
{
	return (int)a[c] - (int)b[c];
}

// Whether every channel of `a` is within `by` of `b`.
static bool within(const uint8_t *a, const uint8_t *b, int by)
{
	for (int c = 0; c < 3; c++)
		if (abs(rise(a, b, c)) > by)
			return false;
	return true;
}

// Whether `a` is at least TINT/255 redder and bluer than `b`, its green rising
// less than half its red and its blue.
static bool purple_tinted(const uint8_t *a, const uint8_t *b)
{
	return rise(a, b, 0) >= TINT && rise(a, b, 2) >= TINT &&
	       2 * rise(a, b, 1) < rise(a, b, 0) &&
	       2 * rise(a, b, 1) < rise(a, b, 2);
}

// `what`, then each of the `count` pixels of `rgb` against `ref`'s.
static void print_reads(const char *what, int count, const uint8_t (*rgb)[3],
			const uint8_t (*ref)[3])
{
	for (int i = 0; i < count; i++)
		printf("%s %d: %d %d %d against %d %d %d\n", what, i, rgb[i][0],
		       rgb[i][1], rgb[i][2], ref[i][0], ref[i][1], ref[i][2]);
}

// The first case of the header.
static void a_small_box_tints_the_ground_beside_it(scene *s,
						   const voe_3d_shapes *shapes)
{
	voe_math_float3 at[3];
	uint8_t bounced[3][3];
	uint8_t reference[3][3];
	uint8_t stronger[1][3];
	uint8_t back[1][3];
	scene plain;
	bool near, far, foot, more, same;

	a_purple_world(s, shapes);
	at[0] = beside(s, 0.25f);
	at[1] = beside(s, 3.0f);
	at[2] = beside(s, -0.25f);
	looks(s, 3, at, bounced);
	plain = a_low_reference(s, shapes);
	looks(&plain, 3, at, reference);
	bounce_strength(s, 5.0f);
	VOE_TEST_CHECK(settles(s));
	looks(s, 1, at, stronger);
	bounce_strength(s, 2.0f);
	VOE_TEST_CHECK(settles(s));
	looks(s, 1, at, back);

	near = purple_tinted(bounced[0], reference[0]);
	far = 2 * rise(bounced[1], reference[1], 0) < TINT &&
	      2 * rise(bounced[1], reference[1], 2) < TINT;
	foot = within(bounced[2], reference[2], SHADOW);
	more = rise(stronger[0], bounced[0], 0) >= 1 &&
	       rise(stronger[0], bounced[0], 2) >= 1;
	same = within(back[0], bounced[0], 1);
	if (!(near && far && foot && more && same)) {
		print_reads("near, far, foot", 3, bounced, reference);
		print_reads("strength 5, back at 2", 1, stronger, back);
	}
	VOE_TEST_CHECK(near);
	VOE_TEST_CHECK(far);
	VOE_TEST_CHECK(foot);
	VOE_TEST_CHECK(more);
	VOE_TEST_CHECK(same);
}

// The second case of the header.
static void a_slab_reddens_the_box_face(scene *s, const voe_3d_shapes *shapes)
{
	voe_math_float3 face;
	voe_scene_transform placed;
	voe_ecs_entity slab;
	uint8_t before[1][3];
	uint8_t after[1][3];

	a_purple_world(s, shapes);
	face = beside(s, 0.0f);
	face.y = 0.5f;
	bounce_strength(s, 5.0f);
	VOE_TEST_CHECK(settles(s));
	looks(s, 1, &face, before);
	slab = add_a_shape(s->world,
			   (voe_math_double3){ face.x + 1.5, -1.0, face.z },
			   (voe_math_float3){ 2.0f, 0.2f, 1.0f }, RED);
	voe_3d_shape_system_run(s->world, shapes);
	placed = *voe_scene_transform_get(s->world, slab);
	placed.position.y = 0.1;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ slab, placed }));
	VOE_TEST_CHECK(settles(s));
	looks(s, 1, &face, after);
	if (rise(after[0], before[0], 0) < TINT)
		print_reads("face with the slab", 1, after, before);
	VOE_TEST_CHECK(rise(after[0], before[0], 0) >= TINT);
}

// The third case of the header.
static void a_moved_box_leaves_no_ring(scene *s, const voe_3d_shapes *shapes)
{
	voe_math_float3 at[2 + RING];
	uint8_t bounced[2 + RING][3];
	uint8_t reference[2 + RING][3];
	voe_scene_transform moved;
	scene plain;
	bool near;
	bool even = true;

	a_purple_world(s, shapes);
	moved = *voe_scene_transform_get(s->world, s->box);
	at[1] = beside(s, 0.25f);
	for (int i = 0; i < RING; i++) {
		float turn = 6.2831853f * (float)i / (float)RING;

		at[2 + i] = (voe_math_float3){
			(float)moved.position.x + 2.0f * cosf(turn), 0.0f,
			(float)moved.position.z + 2.0f * sinf(turn) };
	}
	moved.position.x += MOVE;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ s->box, moved }));
	VOE_TEST_CHECK(settles(s));
	at[0] = beside(s, 0.25f);
	looks(s, 2 + RING, at, bounced);
	plain = a_low_reference(s, shapes);
	looks(&plain, 2 + RING, at, reference);

	near = purple_tinted(bounced[0], reference[0]);
	for (int i = 1; i < 2 + RING; i++)
		even = even && within(bounced[i], reference[i], EVEN);
	if (!(near && even))
		print_reads("new near, old near, ring", 2 + RING, bounced,
			    reference);
	VOE_TEST_CHECK(near);
	VOE_TEST_CHECK(even);
}

// The fourth case of the header.
static void a_recoloured_box_tints_its_new_colour(scene *s,
						  const voe_3d_shapes *shapes)
{
	voe_3d_shape shape;
	voe_math_float3 near;
	uint8_t bounced[1][3];
	uint8_t reference[1][3];
	scene plain;
	bool green;

	a_purple_world(s, shapes);
	shape = *voe_3d_shape_get(s->world, s->box);
	shape.colour = GREEN;
	VOE_TEST_CHECK(voe_3d_shape_submit(
		s->world, (voe_3d_shape_intent){ .entity = s->box, .shape = shape }));
	voe_3d_shape_system_run(s->world, shapes);
	(void)a_full_frame(s);
	voe_3d_shape_system_run(s->world, shapes);
	VOE_TEST_CHECK(settles(s));
	near = beside(s, 0.25f);
	looks(s, 1, &near, bounced);
	plain = a_low_reference(s, shapes);
	looks(&plain, 1, &near, reference);

	green = rise(bounced[0], reference[0], 1) >= TINT &&
		2 * rise(bounced[0], reference[0], 0) < TINT;
	if (!green)
		print_reads("recoloured near", 1, bounced, reference);
	VOE_TEST_CHECK(green);
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

// The last case of the header.
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

// `run` on a new device and arenas. Returns false when the device skips.
static bool on_a_device_of_its_own(void (*run)(scene *, const voe_3d_shapes *))
{
	scene s = { .keep = voe_base_arena_new(SCRATCH),
		    .scratch = voe_base_arena_new(SCRATCH) };
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_3d_shapes shapes;
	bool ran = false;

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
		run(&s, &shapes);
		ran = true;
	}
	if (s.device != NULL)
		voe_render_device_destroy(s.device);
	voe_base_arena_destroy(s.scratch);
	voe_base_arena_destroy(s.keep);
	return ran;
}

int main(void)
{
	void (*const cases[])(scene *, const voe_3d_shapes *) = {
		a_small_box_tints_the_ground_beside_it,
		a_slab_reddens_the_box_face,
		a_moved_box_leaves_no_ring,
		a_recoloured_box_tints_its_new_colour,
		a_whole_relight_after_a_move_keeps_the_tint,
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
		if (!on_a_device_of_its_own(cases[i]))
			break;
	return voe_test_result();
}
