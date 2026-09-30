// The sound system, end to end with a mixer and WAVs this test writes, as
// tests/mixer.c does: a looping sound on a thing plays after a run; a camera
// at the origin looking down -Z hears a thing at +X louder right; a stop
// control silences it; a one-shot run past its end stops playing; a thing
// destroyed through the structural queue has its voice stopped by the next
// run's sweep; and a run with no mixer applies a replace and starts nothing.
//
// Every clip is a constant mono 16-bit one, so a side's loudness is read
// straight off the mix. Opens no device.
#include <audio/sound_system.h>

#include <base/arena.h>

#include <ecs/structure.h>

#include <platform/file.h>
#include <platform/folder.h>

#include <scene/camera_system.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define FOLDER "audio_sound_system_test"
#define SCRATCH (256 * 1024)
#define ENTITIES 4
#define FRAMES 64
#define PI 3.14159265358979f

static float out[FRAMES * VOE_PLATFORM_SOUND_CHANNELS];

static void put_le(uint8_t *at, uint32_t value, uint32_t bytes)
{
	for (uint32_t i = 0; i < bytes; i++)
		at[i] = (uint8_t)(value >> (8 * i));
}

// A mono 16-bit 48 kHz PCM WAV of frames samples, each value, at name.
static void write_wav(const char *name, uint32_t frames, int16_t value)
{
	static uint8_t bytes[44 + 2 * 4096];
	const uint32_t data = frames * 2;
	char path[256];

	VOE_TEST_CHECK(frames <= 4096);
	memcpy(bytes, "RIFF", 4);
	put_le(bytes + 4, 36 + data, 4);
	memcpy(bytes + 8, "WAVEfmt ", 8);
	put_le(bytes + 16, 16, 4);
	put_le(bytes + 20, 1, 2);
	put_le(bytes + 22, 1, 2);
	put_le(bytes + 24, 48000, 4);
	put_le(bytes + 28, 48000 * 2, 4);
	put_le(bytes + 32, 2, 2);
	put_le(bytes + 34, 16, 2);
	memcpy(bytes + 36, "data", 4);
	put_le(bytes + 40, data, 4);
	for (uint32_t i = 0; i < frames; i++)
		put_le(bytes + 44 + 2 * i, (uint16_t)value, 2);
	snprintf(path, sizeof(path), "%s/%s", FOLDER, name);
	VOE_TEST_CHECK(voe_platform_file_write(path, bytes, 44 + data, NULL));
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
		.entities = ENTITIES, .component_types = 4, .intent_types = 4,
		.structure_requests = 4 * ENTITIES, .structure_bytes = 1024 });

	voe_scene_transform_register(world, ENTITIES);
	voe_scene_camera_register(world, 1);
	voe_audio_sound_register(world, ENTITIES);
	return world;
}

// A camera at the origin, unrotated, so looking down -Z, 90 degrees high.
static void a_camera(voe_ecs_world *world)
{
	voe_ecs_entity entity;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
		.rotation = { 0, 0, 0, 1 }, .scale = { 1, 1, 1 } }));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, (voe_scene_camera){
		.fov_y = PI / 2, .near_plane = 0.1f, .far_plane = 100 }));
}

// A thing at where carrying path, playing.
static voe_ecs_entity a_thing(voe_ecs_world *world, voe_math_double3 where,
			      const char *path, bool loop)
{
	voe_ecs_entity entity = { 0 };
	voe_audio_sound sound = { .playing = true, .loop = loop, .volume = 1, .pitch = 1 };

	strcpy(sound.path, path);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, (voe_scene_transform){
		.position = where, .rotation = { 0, 0, 0, 1 }, .scale = { 1, 1, 1 } }));
	VOE_TEST_CHECK(voe_audio_sound_add(world, entity, sound));
	return entity;
}

static voe_audio_voice voice_of(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_audio_sound_voice *row = voe_audio_sound_voice_get(world, entity);

	VOE_TEST_CHECK(row != NULL);
	return row != NULL ? row->voice : (voe_audio_voice){ 0 };
}

static void a_loop_plays_louder_right_until_stopped(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);
	voe_ecs_entity thing;

	write_wav("hum.wav", 100, 8192);
	a_camera(world);
	thing = a_thing(world, (voe_math_double3){ 3, 0, -3 }, "hum.wav", true);
	voe_audio_sound_system_run(world, mixer, 1.0f);
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, voice_of(world, thing)));
	voe_audio_mixer_mix(mixer, out, FRAMES);
	VOE_TEST_CHECK(out[1] > out[0]);
	VOE_TEST_CHECK(out[2 * FRAMES - 1] > out[2 * FRAMES - 2]);

	VOE_TEST_CHECK(voe_audio_sound_control_submit(world, (voe_audio_sound_control){
		.entity = thing, .kind = VOE_AUDIO_SOUND_STOP }));
	voe_audio_sound_system_run(world, mixer, 1.0f);
	VOE_TEST_CHECK(!voe_audio_sound_get(world, thing)->playing);
	voe_audio_mixer_mix(mixer, out, FRAMES);
	for (uint32_t i = 0; i < 2 * FRAMES; i++)
		VOE_TEST_CHECK_FLOAT(out[i], 0.0f, 0.0f);
	voe_audio_mixer_destroy(mixer);
	voe_base_arena_clear(arena);
}

static void a_one_shot_past_its_end_is_not_playing(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);
	voe_ecs_entity thing;

	write_wav("blip.wav", 10, 8192);
	a_camera(world);
	thing = a_thing(world, (voe_math_double3){ 0, 0, -3 }, "blip.wav", false);
	voe_audio_sound_system_run(world, mixer, 1.0f);
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, voice_of(world, thing)));
	voe_audio_mixer_mix(mixer, out, FRAMES);
	voe_audio_sound_system_run(world, mixer, 1.0f);
	VOE_TEST_CHECK(!voe_audio_sound_get(world, thing)->playing);
	VOE_TEST_CHECK(voice_of(world, thing).id == 0);
	voe_audio_mixer_destroy(mixer);
	voe_base_arena_clear(arena);
}

static void a_destroyed_thing_is_swept(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);
	voe_ecs_entity thing;
	voe_audio_voice voice;

	write_wav("hum.wav", 100, 8192);
	a_camera(world);
	thing = a_thing(world, (voe_math_double3){ 0, 0, -3 }, "hum.wav", true);
	voe_audio_sound_system_run(world, mixer, 1.0f);
	voice = voice_of(world, thing);
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, voice));
	VOE_TEST_CHECK(voe_ecs_structure_destroy(world, thing));
	voe_ecs_structure_apply(world);
	voe_audio_sound_system_run(world, mixer, 1.0f);
	VOE_TEST_CHECK(!voe_audio_mixer_playing(mixer, voice));
	VOE_TEST_CHECK_INT(voe_audio_sound_voice_count(world), 0);
	voe_audio_mixer_destroy(mixer);
	voe_base_arena_clear(arena);
}

static void no_mixer_applies_a_replace_and_starts_nothing(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity thing = a_thing(world, (voe_math_double3){ 0 }, "", true);
	voe_audio_sound_intent intent = { .entity = thing,
		.sound = { .playing = true, .loop = true, .volume = 0.5f, .pitch = 1 } };

	strcpy(intent.sound.path, "hum.wav");
	VOE_TEST_CHECK(voe_audio_sound_submit(world, intent));
	voe_audio_sound_system_run(world, NULL, 1.0f);
	VOE_TEST_CHECK(strcmp(voe_audio_sound_get(world, thing)->path, "hum.wav") == 0);
	VOE_TEST_CHECK(voe_audio_sound_get(world, thing)->volume == 0.5f);
	VOE_TEST_CHECK(voice_of(world, thing).id == 0);
	voe_base_arena_clear(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_base_error error = VOE_BASE_OK;

	// A folder left by an earlier run is taken as it is.
	if (!voe_platform_folder_create(FOLDER, &error))
		VOE_TEST_CHECK(error == VOE_BASE_ERROR_REFUSED);
	a_loop_plays_louder_right_until_stopped(arena);
	a_one_shot_past_its_end_is_not_playing(arena);
	a_destroyed_thing_is_swept(arena);
	no_mixer_applies_a_replace_and_starts_nothing(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
