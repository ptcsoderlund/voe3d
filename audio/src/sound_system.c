// The sound system's run: the two drains, the voice rows added and dropped,
// the listener from the camera, each row reconciled with its voice and the
// sweep. See audio/sound_system.h for what a run does and in which order.
//
// A ROW IS CHANGED AS A COPY, set back whole: the table hands out no writable
// row, as in 3d/src/emitter_system.c.
//
// THE CORRECTED-PATH RUN AND COUNT ARE FILE-SCOPE STATICS, per process, the
// trade 3d/src/emitter_system.c makes.
#include <audio/sound_system.h>

#include <base/assert.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/intent.h>

#include <math/float4x4.h>

#include <scene/camera_component.h>
#include <scene/transform_component.h>

#include <inttypes.h>
#include <math.h>
#include <string.h>

static bool in_corrected_run;
static uint32_t corrected_run_count;

static voe_ecs_type sound_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_audio_sound_key);
}

static voe_ecs_type voice_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_audio_sound_voice_key);
}

// Applies every replace in submission order, a path with no end cut and
// reported as the emitter's texture is.
static void drain_replaces(voe_ecs_world *world)
{
	const voe_ecs_type type = sound_type(world);
	const voe_ecs_intent queue = voe_ecs_component_replace(world, type).intent;
	const voe_audio_sound_intent *intents = voe_ecs_intent_queue(world, queue);
	const uint32_t count = voe_ecs_intent_count(world, queue);
	uint32_t corrected = 0;

	for (uint32_t i = 0; i < count; i++) {
		voe_audio_sound row = intents[i].sound;

		if (voe_ecs_component_get(world, type, intents[i].entity) == NULL)
			continue;
		if (memchr(row.path, '\0', VOE_AUDIO_SOUND_PATH) == NULL) {
			row.path[VOE_AUDIO_SOUND_PATH - 1] = '\0';
			if (!in_corrected_run && corrected == 0)
				VOE_BASE_WARNING(
					"audio",
					"voe_audio_sound: entity %" PRIu32 "v%" PRIu32
					": path of %d bytes with no end, cut to \"%s\"",
					intents[i].entity.index,
					intents[i].entity.generation,
					VOE_AUDIO_SOUND_PATH, row.path);
			corrected++;
		}
		(void)voe_ecs_component_set(world, type, intents[i].entity, &row);
	}

	if (corrected > 0) {
		in_corrected_run = true;
		corrected_run_count += corrected;
	} else if (in_corrected_run) {
		VOE_BASE_WARNING("audio",
				 "voe_audio_sound: %" PRIu32
				 " intents corrected in that run",
				 corrected_run_count);
		in_corrected_run = false;
		corrected_run_count = 0;
	}
	voe_ecs_intent_clear(world, queue);
}

// The entity's voice row, added empty when it has none. Asserts the table has
// room, since it is registered with the sound's capacity.
static voe_audio_sound_voice voice_of(voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_audio_sound_voice *own = voe_audio_sound_voice_get(world, entity);
	const voe_audio_sound_voice row = { 0 };

	if (own != NULL)
		return *own;
	VOE_BASE_ASSERT(voe_ecs_component_add(world, voice_type(world), entity, &row),
			"the sound voice table is smaller than the sound table");
	return row;
}

// Applies every control in submission order to the sound and its voice row.
static void drain_controls(voe_ecs_world *world)
{
	const voe_ecs_type type = sound_type(world);
	const voe_ecs_intent queue =
		voe_ecs_intent_type(world, &voe_audio_sound_control_key);
	const voe_audio_sound_control *controls = voe_ecs_intent_queue(world, queue);
	const uint32_t count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		const voe_audio_sound *own = voe_audio_sound_get(world, controls[i].entity);
		voe_audio_sound sound;
		voe_audio_sound_voice voice;

		if (own == NULL)
			continue;
		sound = *own;
		voice = voice_of(world, controls[i].entity);
		if (controls[i].kind == VOE_AUDIO_SOUND_PLAY) {
			sound.playing = true;
			voice.restart = true;
		} else if (controls[i].kind == VOE_AUDIO_SOUND_STOP) {
			sound.playing = false;
		} else if (controls[i].kind == VOE_AUDIO_SOUND_TUNE) {
			sound.volume = controls[i].volume;
			sound.pitch = controls[i].pitch;
		}
		(void)voe_ecs_component_set(world, type, controls[i].entity, &sound);
		(void)voe_ecs_component_set(world, voice_type(world),
					    controls[i].entity, &voice);
	}
	voe_ecs_intent_clear(world, queue);
}

// Drops the voice rows whose sound is gone, walking back so a swapped-in row
// has been seen, then adds one to each sound lacking it.
static void add_and_drop_rows(voe_ecs_world *world)
{
	const voe_ecs_type voices = voice_type(world);
	const voe_ecs_entity *owners = voe_audio_sound_voice_entities(world);
	const voe_ecs_entity *entities = voe_audio_sound_entities(world);
	const uint32_t count = voe_audio_sound_count(world);

	for (uint32_t i = voe_audio_sound_voice_count(world); i > 0; i--)
		if (voe_audio_sound_get(world, owners[i - 1]) == NULL)
			(void)voe_ecs_component_remove(world, voices, owners[i - 1]);

	for (uint32_t i = 0; i < count; i++)
		(void)voice_of(world, entities[i]);
}

// Hears from the first camera whose entity has a transform, or from nobody.
static void listen(const voe_ecs_world *world, voe_audio_mixer *mixer, float aspect)
{
	const voe_scene_camera *cameras = voe_scene_camera_rows(world);
	const voe_ecs_entity *entities = voe_scene_camera_entities(world);
	const uint32_t count = voe_scene_camera_count(world);

	for (uint32_t i = 0; i < count; i++) {
		voe_scene_transform pose;
		voe_math_float4x4 turn;
		voe_audio_listener ear;

		if (voe_scene_transform_get(world, entities[i]) == NULL)
			continue;
		pose = voe_scene_transform_world(world, entities[i]);
		turn = voe_math_float4x4_from_quat(pose.rotation);
		ear = voe_audio_listener_make(
			pose.position,
			voe_math_float4x4_transform_dir(turn, (voe_math_float3){ 1, 0, 0 }),
			voe_math_float4x4_transform_dir(turn, (voe_math_float3){ 0, 0, -1 }),
			tanf(cameras[i].fov_y * 0.5f) * aspect);
		voe_audio_mixer_listen(mixer, &ear);
		return;
	}
	voe_audio_mixer_listen(mixer, NULL);
}

// Brings one row's voice in line with its sound, and the row with the voice.
static void reconcile(voe_ecs_world *world, voe_audio_mixer *mixer,
		      voe_ecs_entity entity, voe_audio_sound sound)
{
	voe_audio_sound_voice voice = *voe_audio_sound_voice_get(world, entity);
	const bool placed = voe_scene_transform_get(world, entity) != NULL;
	const voe_math_double3 where =
		placed ? voe_scene_transform_world(world, entity).position :
			 (voe_math_double3){ 0 };
	const bool live = voe_audio_mixer_playing(mixer, voice.voice);

	if (voice.voice.id != 0 && !live && !sound.loop && !voice.restart) {
		sound.playing = false;
		(void)voe_ecs_component_set(world, sound_type(world), entity, &sound);
	}
	if (!sound.playing) {
		voe_audio_mixer_stop(mixer, voice.voice);
		voice = (voe_audio_sound_voice){ 0 };
	} else if (voice.restart || !live) {
		voe_audio_mixer_stop(mixer, voice.voice);
		voice.voice = voe_audio_mixer_start(mixer, (voe_audio_start){
			.path = sound.path, .loop = sound.loop, .held = true,
			.volume = sound.volume, .pitch = sound.pitch,
			.placed = placed, .where = where });
		voice.restart = false;
	} else {
		voe_audio_mixer_tune(mixer, voice.voice, sound.volume, sound.pitch);
		if (placed)
			voe_audio_mixer_move(mixer, voice.voice, where);
	}
	(void)voe_ecs_component_set(world, voice_type(world), entity, &voice);
}

void voe_audio_sound_system_run(voe_ecs_world *world, voe_audio_mixer *mixer,
				float aspect)
{
	const voe_audio_sound *sounds;
	const voe_ecs_entity *entities;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the sound system on no world");
	VOE_BASE_ASSERT(aspect > 0.0f, "the sound system needs a positive aspect");

	drain_replaces(world);
	drain_controls(world);
	add_and_drop_rows(world);
	if (mixer == NULL)
		return;

	listen(world, mixer, aspect);
	sounds = voe_audio_sound_rows(world);
	entities = voe_audio_sound_entities(world);
	count = voe_audio_sound_count(world);
	for (uint32_t i = 0; i < count; i++)
		reconcile(world, mixer, entities[i], sounds[i]);
	voe_audio_mixer_sweep(mixer);
}
