// The fixed steps on a game world: one step's time runs one step and draws a
// whole step behind, two and a half run two and draw half behind, a second
// runs the four at most and keeps under one step, and a capsule over a box
// floor, pulled down by the test's systems, lands and stands after 60 steps,
// and a follower set to a falling body's position after the move is where
// the body is by the end of the same step, and an emitter of rate 60 on a
// thing with a transform holds live particles after 60 steps, and a looping
// sound on a thing, a WAV the test writes in its working directory, plays
// through a mixer on that folder after two steps, and one step advances a
// water's clock, added by the step before, by the step, and a point light's
// replace, as game code fading it sends, takes in one step.
// Needs no window, no graphics card and no sound device: nothing is shaped,
// so the shapes are zeros, and the mixer is never pumped.
#include <game/steps.h>

#include <game/world.h>

#include <audio/sound_component.h>

#include <3d/emitter_component.h>
#include <3d/water_component.h>

#include <base/arena.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <physics/body_component.h>
#include <physics/body_system.h>
#include <physics/collider_component.h>
#include <physics/collider_system.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <string.h>

#define GRAVITY 9.81f

// Where the test's WAV is written, and its length in frames.
#define SOUND_FOLDER "game_steps_test"
#define WAV_FRAMES 100

// What the stub systems saw, the body they pull down when `falling`, and the
// entity set to the body's position after the move when `following`.
static struct {
	unsigned calls;
	unsigned after_calls;
	bool falling;
	bool following;
	voe_ecs_entity body;
	voe_ecs_entity follower;
} seen;

// Counts its calls; when falling, adds a step of gravity to the body's
// velocity, from rest when it is on the floor, as a project would (0249).
static void systems(const voe_game_project_step *step)
{
	voe_physics_body row;
	float fall = GRAVITY * (float)step->seconds;

	VOE_TEST_CHECK(step->seconds == VOE_GAME_STEP_SECONDS);
	seen.calls++;
	if (!seen.falling)
		return;
	row = *voe_physics_body_get(step->world, seen.body);
	row.velocity.y = row.on_floor ? -fall : row.velocity.y - fall;
	VOE_TEST_CHECK(voe_physics_body_submit(
		step->world, (voe_physics_body_intent){ seen.body, row }));
}

// Counts its calls; when following, submits the follower's transform with
// the body's current position, as a camera follow would (0256).
static void after_move(const voe_game_project_step *step)
{
	voe_scene_transform moved;

	VOE_TEST_CHECK(step->seconds == VOE_GAME_STEP_SECONDS);
	seen.after_calls++;
	if (!seen.following)
		return;
	moved = *voe_scene_transform_get(step->world, seen.follower);
	moved.position =
		voe_scene_transform_get(step->world, seen.body)->position;
	VOE_TEST_CHECK(voe_scene_transform_submit(
		step->world,
		(voe_scene_transform_intent){ seen.follower, moved }));
}

// An entity at `at` with a collider of `kind` and `size`.
static voe_ecs_entity placed(voe_ecs_world *world, voe_math_double3 at,
			     uint32_t kind, voe_math_float3 size)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = at,
				       .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, entity,
		(voe_physics_collider){ .kind = kind, .size = size }));
	return entity;
}

static void steps_are_counted(voe_ecs_world *world,
			      const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	float lag;

	lag = voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 1);
	VOE_TEST_CHECK_FLOAT(lag, 1.0f, 0.0f);

	seen.calls = 0;
	steps = (voe_game_steps){ 0 };
	lag = voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 2.5 * VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 2);
	VOE_TEST_CHECK_FLOAT(lag, 0.5f, 1e-4f);

	seen.calls = 0;
	steps = (voe_game_steps){ 0 };
	lag = voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes, 1.0,
				 systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, VOE_GAME_STEPS_MAX);
	VOE_TEST_CHECK(steps.banked >= 0.0 &&
		       steps.banked < VOE_GAME_STEP_SECONDS);
	VOE_TEST_CHECK(lag > 0.0f && lag <= 1.0f);
}

// A capsule half a metre over a box floor whose top is y = 0.
static void a_body_lands(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };

	(void)placed(world, (voe_math_double3){ 0.0, -0.5, 0.0 },
		     VOE_PHYSICS_COLLIDER_BOX,
		     (voe_math_float3){ 40.0f, 1.0f, 40.0f });
	seen.body = placed(world, (voe_math_double3){ 0.0, 1.5, 0.0 },
			   VOE_PHYSICS_COLLIDER_CAPSULE,
			   (voe_math_float3){ 1.0f, 2.0f, 1.0f });
	VOE_TEST_CHECK(voe_physics_body_add(
		world, seen.body,
		(voe_physics_body){ .step_height = 0.3f,
				    .slope_limit = 0.8f }));
	seen.falling = true;
	seen.calls = 0;
	for (int i = 0; i < 60; i++)
		(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
					 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 60);
	VOE_TEST_CHECK(voe_physics_body_get(world, seen.body)->on_floor);
	VOE_TEST_CHECK_FLOAT(
		(float)voe_scene_transform_get(world, seen.body)->position.y,
		1.0f, 1e-3f);
}

// A capsule in the air far off the floor, and a follower at the origin.
static void a_follower_keeps_up(voe_ecs_world *world,
				const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_math_double3 body_at;
	voe_math_double3 follower_at;

	seen.body = placed(world, (voe_math_double3){ 100.0, 10.0, 0.0 },
			   VOE_PHYSICS_COLLIDER_CAPSULE,
			   (voe_math_float3){ 1.0f, 2.0f, 1.0f });
	VOE_TEST_CHECK(voe_physics_body_add(
		world, seen.body,
		(voe_physics_body){ .step_height = 0.3f,
				    .slope_limit = 0.8f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &seen.follower));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, seen.follower,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	seen.falling = true;
	seen.following = true;
	seen.calls = 0;
	seen.after_calls = 0;
	(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	VOE_TEST_CHECK_INT(seen.calls, 1);
	VOE_TEST_CHECK_INT(seen.after_calls, 1);
	body_at = voe_scene_transform_get(world, seen.body)->position;
	follower_at = voe_scene_transform_get(world, seen.follower)->position;
	VOE_TEST_CHECK(body_at.y < 10.0);
	VOE_TEST_CHECK(follower_at.x == body_at.x &&
		       follower_at.y == body_at.y &&
		       follower_at.z == body_at.z);
}

// An emitter of rate 60 and the default life on a thing at the origin: a
// second of steps later its particles row holds live ones.
static void an_emitter_spawns(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_3d_emitter emitter = *(const voe_3d_emitter *)voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_3d_emitter_key));
	voe_ecs_entity entity = { 0 };
	const voe_3d_particles *particles;

	seen.falling = false;
	seen.following = false;
	emitter.rate = 60.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_emitter_add(world, entity, emitter));
	for (int i = 0; i < 60; i++)
		(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
					 VOE_GAME_STEP_SECONDS, systems, after_move);
	particles = voe_3d_particles_get(world, entity);
	VOE_TEST_CHECK(particles != NULL);
	if (particles != NULL)
		VOE_TEST_CHECK(particles->count > 0);
}

// A water on a thing at the origin: the first step adds its clock and steps
// it, so the second moves it on by exactly one step.
static void a_water_keeps_time(voe_ecs_world *world,
			       const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_3d_water water = *(const voe_3d_water *)voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_3d_water_key));
	voe_ecs_entity entity = { 0 };
	const voe_3d_waves *waves;
	double before;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_3d_water_add(world, entity, water));
	(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	waves = voe_3d_waves_get(world, entity);
	VOE_TEST_CHECK(waves != NULL);
	if (waves == NULL)
		return;
	before = waves->seconds;
	(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	waves = voe_3d_waves_get(world, entity);
	VOE_TEST_CHECK(waves != NULL);
	if (waves != NULL)
		VOE_TEST_CHECK_FLOAT((float)(waves->seconds - before),
				     (float)VOE_GAME_STEP_SECONDS, 1e-6f);
}

// A lamp of intensity 2 and falloff 1 on a thing at the origin, and a replace
// of intensity 0.5 and falloff 3 submitted, as game code fading a light does:
// after one step its row reads the replace's.
static void a_point_light_replace_takes(voe_ecs_world *world,
					const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_ecs_entity entity = { 0 };
	const voe_scene_point_light lamp = { .colour = { 1.0f, 1.0f, 1.0f },
					     .intensity = 2.0f,
					     .range = 5.0f,
					     .falloff = 1.0f };
	voe_scene_point_light faded = lamp;
	const voe_scene_point_light *row;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_point_light_add(world, entity, lamp));
	faded.intensity = 0.5f;
	faded.falloff = 3.0f;
	VOE_TEST_CHECK(voe_scene_point_light_submit(
		world,
		(voe_scene_point_light_intent){ .entity = entity, .light = faded }));
	(void)voe_game_steps_run(&steps, world, NULL, NULL, NULL, shapes,
				 VOE_GAME_STEP_SECONDS, systems, after_move);
	row = voe_scene_point_light_get(world, entity);
	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK_FLOAT(row->intensity, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(row->falloff, 3.0f, 0.0f);
}

static void put_le(uint8_t *at, uint32_t value, uint32_t bytes)
{
	for (uint32_t i = 0; i < bytes; i++)
		at[i] = (uint8_t)(value >> (8 * i));
}

// A constant mono 16-bit 48 kHz PCM WAV of WAV_FRAMES samples at SOUND_FOLDER/hum.wav.
static void write_hum(void)
{
	static uint8_t bytes[44 + 2 * WAV_FRAMES];
	voe_base_error error = VOE_BASE_OK;

	// A folder left by an earlier run is taken as it is.
	if (!voe_platform_folder_create(SOUND_FOLDER, &error))
		VOE_TEST_CHECK(error == VOE_BASE_ERROR_REFUSED);
	memcpy(bytes, "RIFF", 4);
	put_le(bytes + 4, 36 + 2 * WAV_FRAMES, 4);
	memcpy(bytes + 8, "WAVEfmt ", 8);
	put_le(bytes + 16, 16, 4);
	put_le(bytes + 20, 1, 2);
	put_le(bytes + 22, 1, 2);
	put_le(bytes + 24, 48000, 4);
	put_le(bytes + 28, 48000 * 2, 4);
	put_le(bytes + 32, 2, 2);
	put_le(bytes + 34, 16, 2);
	memcpy(bytes + 36, "data", 4);
	put_le(bytes + 40, 2 * WAV_FRAMES, 4);
	for (uint32_t i = 0; i < WAV_FRAMES; i++)
		put_le(bytes + 44 + 2 * i, 8192, 2);
	VOE_TEST_CHECK(voe_platform_file_write(SOUND_FOLDER "/hum.wav", bytes,
					       sizeof(bytes), NULL));
}

// A looping sound on a thing at the origin, two steps through a mixer on the
// folder: its voice row holds a voice the mixer plays.
static void a_sound_plays(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	voe_game_steps steps = { 0 };
	voe_audio_mixer *mixer;
	voe_audio_sound sound = { .playing = true, .loop = true, .volume = 1,
				  .pitch = 1 };
	voe_ecs_entity entity = { 0 };
	const voe_audio_sound_voice *voice;

	write_hum();
	mixer = voe_audio_mixer_new(SOUND_FOLDER);
	strcpy(sound.path, "hum.wav");
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0, 0, 0, 1 },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_audio_sound_add(world, entity, sound));
	(void)voe_game_steps_run(&steps, world, NULL, mixer, NULL, shapes,
				 2 * VOE_GAME_STEP_SECONDS, systems, after_move);
	voice = voe_audio_sound_voice_get(world, entity);
	VOE_TEST_CHECK(voice != NULL);
	if (voice != NULL)
		VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, voice->voice));
	voe_audio_mixer_destroy(mixer);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 22);
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_3d_shapes shapes = { 0 };

	steps_are_counted(world, &shapes);
	a_body_lands(world, &shapes);
	a_follower_keeps_up(world, &shapes);
	an_emitter_spawns(world, &shapes);
	a_sound_plays(world, &shapes);
	a_water_keeps_time(world, &shapes);
	a_point_light_replace_takes(world, &shapes);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
