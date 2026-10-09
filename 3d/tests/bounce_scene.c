// The probe bounce as the editor and the game draw it (feature 051, How to test
// steps 2, 3 and 8). The world, its reference, the frame, the settle and the
// window read are bounce_world.inc's, and its header says how each works;
// here the box is 2 m a side, strongly red, at (0.5, 1, 0.5), off the probe
// lattice's odd metres, the sun on its +x face, and the 1 m nest, its lowest
// cell the eye's less (12, 9, 12), holds the lit side clear of its 2-cell edge
// band (eye cell z at most 10, y at most 6).
//
// THE LIT SIDE (step 2): the ground 0.25 m out from the sunlit +x face has red
// less green above the reference's by at least TINT/255, 3 m out by less than
// half that. THE SHADOW (step 3, 0312, 0327): the ground at the foot of the
// shadowed -x face within SHADOW/255 of the reference in every channel. EVEN
// GROUND (step 2, 0326): five points along x = -6, z -12 to -4, 2 m apart, within
// EVEN/255 of each other; again with the sun 5 degrees higher, each greener.
//
// SETTLING (step 8): unchanged, the next probing frame opens no capture pass;
// the box moved 1 m along x makes the next open one; within BOUND it settles.
//
// TURN (bug 01) AND LOOKING AWAY (0328, 0329): settled, the camera turned 90
// degrees about Y in place for TURNED probing frames, each opening no capture
// pass; then turned 180 degrees and, facing away, the bounce strength set to 2
// and settled, back to 1 and settled. Turned back each time, the lit-side,
// shadow-foot and open-ground pixels are each within 1/255 of before.
// MOVE (bug 03, 0388): the same, the eye nudged 1.5 m along -x, within the 1 m
// nest's two cells; then moved 6 m along -x and 3 m up, settled at each end.
//
// BLOCKED (0347 point 4): settled, a light blocker in both worlds around the
// ground 0.4 to 1.6 m out from the box's sunlit face, not the box. Settled, the
// ground 0.75 m out reads within BLOCKED/255 of the reference, the looked-at
// pixels outside it within BLOCKED/255 of before. Then destroyed and settled;
// the tint coming back is not checked until work order 083 (0393).
//
// FAR (bug 03, 0331): the eye 40 m further back along +z, settled, then TURNED
// probing frames each true; the lit side there at least half TINT/255 redder.
//
// IT NEEDS A GRAPHICS CARD WITH shaderOutputLayer AND SKIPS WITH A REASON
// WITHOUT ONE, as shadows.c does.
#include <math/quat.h>
#include <scene/light_blocker_component.h>
#include <scene/light_system.h>

#include <stdlib.h>

#include "bounce_world.inc"

#define EVEN 2
#define BLOCKED 2
#define OPEN 5
#define TURNED 3
// The pixels TURN compares: the lit side, the shadow's foot and the open ground.
#define LOOKED (2 + OPEN)

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
			(voe_math_float3){ -6.0f, 0.0f, -12.0f + 2.0f * (float)i });

		printf("open %d: %d %d %d\n", i, p[0], p[1], p[2]);
		for (int c = 0; c < 3; c++)
			pixels[i][c] = p[c];
	}
	for (int i = 1; i < OPEN; i++)
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)pixels[i][c] - (int)pixels[0][c]) <=
				       EVEN);
}

// `entity` given `transform` by an intent, applied by the next frame's step.
static void place(scene *s, voe_ecs_entity entity, voe_scene_transform transform)
{
	VOE_TEST_CHECK(voe_scene_transform_submit(
		s->world, (voe_scene_transform_intent){ entity, transform }));
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
		at[2 + i] = (voe_math_float3){ -6.0f, 0.0f, -12.0f + 2.0f * (float)i };
	for (int i = 0; i < LOOKED; i++) {
		const uint8_t *p = pixel_at(picture, frame, at[i]);

		for (int c = 0; c < 3; c++)
			pixels[i][c] = p[c];
	}
	VOE_TEST_CHECK(LOOKED == 2 + OPEN);
}

// The camera at `away` for TURNED probing frames, each opening no capture pass,
// then back; with `settle`, settled at `away` and again once back instead:
// every looked-at pixel within 1/255 of before; `what` on each line.
static void away_and_back_changes_nothing(scene *s, voe_scene_transform away,
					  bool settle, const char *what)
{
	voe_scene_transform pose = *voe_scene_transform_get(s->world, s->camera);
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);
	uint8_t before[LOOKED][3];
	uint8_t after[LOOKED][3];

	the_pixels_looked_at(&picture, &frame, before);
	place(s, s->camera, away);
	if (settle)
		VOE_TEST_CHECK(settles(s));
	else
		for (int f = 0; f < TURNED; f++)
			VOE_TEST_CHECK(!a_capture_was_wanted(s));
	place(s, s->camera, pose);
	if (settle)
		VOE_TEST_CHECK(settles(s));
	frame = a_full_frame(s);
	picture = read_window(s);
	the_pixels_looked_at(&picture, &frame, after);
	for (int i = 0; i < LOOKED; i++) {
		printf("%s %d: %d %d %d against %d %d %d\n", what, i,
		       after[i][0], after[i][1], after[i][2], before[i][0],
		       before[i][1], before[i][2]);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)after[i][c] - (int)before[i][c]) <= 1);
	}
}

// TURN: a quarter turn about Y in place and back.
static void turning_moves_nothing(scene *s)
{
	voe_scene_transform turned = *voe_scene_transform_get(s->world, s->camera);

	turned.rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle((voe_math_float3){ 0.0f, 1.0f, 0.0f },
					      1.5707963f),
		turned.rotation);
	away_and_back_changes_nothing(s, turned, false, "turned");
}

// MOVE: 1.5 m along -x and back, within the 1 m nest's two cells; then 6 m
// along -x and 3 m up and back, settled at each.
static void moving_moves_nothing(scene *s)
{
	voe_scene_transform nudged = *voe_scene_transform_get(s->world, s->camera);
	voe_scene_transform moved = nudged;

	nudged.position.x -= 1.5;
	away_and_back_changes_nothing(s, nudged, false, "nudged");
	moved.position.x -= 6.0;
	moved.position.y += 3.0;
	away_and_back_changes_nothing(s, moved, true, "moved");
}

// A light blocker of `size` at `at`, unturned; the entity.
static voe_ecs_entity add_a_blocker(voe_ecs_world *world, voe_math_double3 at,
				    voe_math_float3 size)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = at,
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, entity, (voe_scene_light_blocker){ .size = size }));
	return entity;
}

// BLOCKED: the patch keeps the bounce out while the blocker stands, the ground
// outside it unchanged. The blocker is then destroyed and settled, since FAR
// reads the lit side inside its box; that the tint comes back is set aside
// until work order 083 (0393).
static void a_blocker_keeps_the_bounce_out(scene *s, scene *plain)
{
	const voe_scene_transform *box = voe_scene_transform_get(s->world, s->box);
	voe_math_double3 centre = { box->position.x + 2.0, 0.0, box->position.z };
	voe_math_float3 size = { 1.2f, 0.6f, 2.0f };
	voe_math_float3 patch = { (float)box->position.x + 1.75f, 0.0f,
				  (float)box->position.z };
	voe_3d_frame frame = a_full_frame(s);
	voe_render_picture picture = read_window(s);
	voe_render_picture reference;
	voe_ecs_entity blocker;
	voe_ecs_entity plain_blocker;
	uint8_t before[LOOKED][3];
	uint8_t after[LOOKED][3];
	uint8_t tinted[3];
	const uint8_t *p;
	const uint8_t *r;

	the_pixels_looked_at(&picture, &frame, before);
	p = pixel_at(&picture, &frame, patch);
	for (int c = 0; c < 3; c++)
		tinted[c] = p[c];
	blocker = add_a_blocker(s->world, centre, size);
	plain_blocker = add_a_blocker(plain->world, centre, size);
	VOE_TEST_CHECK(settles(s));
	frame = a_full_frame(s);
	picture = read_window(s);
	(void)a_full_frame(plain);
	(void)a_full_frame(plain);
	reference = read_window(plain);
	p = pixel_at(&picture, &frame, patch);
	r = pixel_at(&reference, &frame, patch);
	printf("blocked patch: %d %d %d against %d %d %d, tinted %d %d %d\n",
	       p[0], p[1], p[2], r[0], r[1], r[2], tinted[0], tinted[1],
	       tinted[2]);
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)p[c] - (int)r[c]) <= BLOCKED);
	the_pixels_looked_at(&picture, &frame, after);
	for (int i = 0; i < LOOKED; i++) {
		printf("blocked %d: %d %d %d against %d %d %d\n", i, after[i][0],
		       after[i][1], after[i][2], before[i][0], before[i][1],
		       before[i][2]);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs((int)after[i][c] - (int)before[i][c]) <=
				       BLOCKED);
	}

	voe_ecs_entity_destroy(s->world, blocker);
	voe_ecs_entity_destroy(plain->world, plain_blocker);
	VOE_TEST_CHECK(settles(s));
}

// FAR: both worlds' eyes 40 m further back along +z, looking down at the
// origin; no capture pass opens there, and the ground 0.25 m out from the
// box's +x face where it now stands, from there, is redder than the
// reference's by at least half of TINT/255.
static void far_keeps_the_bounce(scene *s, scene *plain)
{
	voe_scene_transform far = *voe_scene_transform_get(s->world, s->camera);
	float pitch = atan2f((float)far.position.y, (float)far.position.z + 40.0f);
	const voe_scene_transform *box = voe_scene_transform_get(s->world, s->box);
	voe_math_float3 lit_side = { (float)box->position.x + 1.25f, 0.0f,
				     (float)box->position.z };
	voe_render_picture bounced;
	voe_render_picture reference;
	voe_3d_frame frame;

	far.position.z += 40.0;
	far.rotation = (voe_math_quat){ -sinf(pitch * 0.5f), 0.0f, 0.0f,
					cosf(pitch * 0.5f) };
	place(s, s->camera, far);
	place(plain, plain->camera, far);
	VOE_TEST_CHECK(settles(s));
	for (int f = 0; f < TURNED; f++)
		VOE_TEST_CHECK(!a_capture_was_wanted(s));
	frame = a_full_frame(s);
	bounced = read_window(s);
	(void)a_full_frame(plain);
	(void)a_full_frame(plain);
	reference = read_window(plain);
	VOE_TEST_CHECK(2 * redder_by(&bounced, &reference, &frame, lit_side) >=
		       TINT);
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
	scene plain;
	voe_render_picture bounced;
	voe_render_picture reference;
	voe_render_picture turned;
	voe_scene_transform moved;
	voe_3d_frame frame;
	uint8_t before[OPEN][3];
	uint8_t after[OPEN][3];

	a_world(s, shapes, 1, (voe_math_double3){ 0.5, 1.0, 0.5 },
		(voe_math_float3){ 2.0f, 2.0f, 2.0f },
		(voe_math_float3){ 1.0f, 0.02f, 0.02f });
	plain = a_reference_world(s, shapes);
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
	moved = *voe_scene_transform_get(s->world, s->box);
	moved.position.x += 1.0;
	place(s, s->box, moved);
	VOE_TEST_CHECK(a_capture_was_wanted(s));
	VOE_TEST_CHECK(settles(s));
	a_blocker_keeps_the_bounce_out(s, &plain);
	turning_moves_nothing(s);
	looking_away_relights_the_same(s);
	moving_moves_nothing(s);
	far_keeps_the_bounce(s, &plain);
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
