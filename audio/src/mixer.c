// The mixer behind audio/mixer.h: a list of clips found by path, a fixed set of
// voices each pointing into one clip, and a mix that sums them.
//
// Two arenas. `keep` holds the mixer itself, the folder, every clip's path and
// every clip's converted samples, and lives until _destroy. `scratch` holds a
// file's bytes, its joined path and its decoded samples during one load, and is
// rewound when the load ends, succeeded or not.
//
// A clip is stored as 48 kHz stereo float, converted once at load, so mixing is
// a plain add with no per-frame conversion. A failed path is a clip too, marked
// failed, which is what keeps it from being read or reported twice.
//
// Each start in a slot bumps that slot's generation, which is what makes an
// older handle to it stale. Reading and summing one voice is voice.c's.
#include <audio/mixer.h>

#include "voice.h"

#include <assets/sound.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

#include <string.h>

#define MIXER_NO_CLIP UINT32_MAX
#define MIXER_SCRATCH_BLOCK (1u << 20)
#define MIXER_KEEP_BLOCK (1u << 20)
#define MIXER_SLOT_BITS 8u
#define MIXER_GENERATION_MASK ((1u << (32u - MIXER_SLOT_BITS)) - 1u)
#define MIXER_PITCH_MIN 0.25f
#define MIXER_PITCH_MAX 4.0f
#define MIXER_VOLUME_MAX 4.0f

static_assert(VOE_AUDIO_VOICES <= (1u << MIXER_SLOT_BITS), "a slot fits its bits of a handle");

struct voe_audio_mixer {
	voe_base_arena *keep;
	voe_base_arena *scratch;
	const char *folder;
	struct mixer_clip clips[VOE_AUDIO_CLIPS];
	uint32_t clip_count;
	bool full_reported;
	struct mixer_voice voices[VOE_AUDIO_VOICES];
	uint64_t plays;
	float pumped[VOE_PLATFORM_SOUND_QUEUE * MIXER_CHANNELS];
};

static const char *copy_text(voe_base_arena *arena, const char *text)
{
	const size_t size = strlen(text) + 1;
	char *copy = voe_base_arena_push(arena, size);

	memcpy(copy, text, size);
	return copy;
}

voe_audio_mixer *voe_audio_mixer_new(const char *folder)
{
	VOE_BASE_ASSERT(folder != NULL, "a mixer needs the folder paths are read from");
	voe_base_arena *keep = voe_base_arena_new(MIXER_KEEP_BLOCK);
	voe_audio_mixer *mixer = voe_base_arena_push(keep, sizeof(*mixer));

	mixer->keep = keep;
	mixer->scratch = voe_base_arena_new(MIXER_SCRATCH_BLOCK);
	mixer->folder = copy_text(keep, folder);
	return mixer;
}

void voe_audio_mixer_destroy(voe_audio_mixer *mixer)
{
	if (mixer == NULL)
		return;
	// The mixer lives in `keep`, so that arena goes last.
	voe_base_arena *keep = mixer->keep;

	voe_base_arena_destroy(mixer->scratch);
	voe_base_arena_destroy(keep);
}

// One sample of the decoded sound at a frame and output channel; a mono sound
// gives its one channel to both sides.
static float source_sample(const voe_assets_sound *sound, uint64_t frame,
			   uint32_t channel)
{
	const uint32_t from = sound->channels == 1 ? 0 : channel;

	return sound->samples[frame * sound->channels + from];
}

// Reads, decodes and converts the file behind clip. False, reported, when any
// step fails; the scratch arena is rewound either way.
static bool load_clip(voe_audio_mixer *mixer, struct mixer_clip *clip)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(mixer->scratch);
	const char *full = voe_platform_path_join(mixer->scratch, mixer->folder,
						  clip->path);
	size_t size = 0;
	const uint8_t *bytes = voe_platform_file_read(full, mixer->scratch, &size, NULL);
	voe_assets_sound sound = { 0 };
	bool loaded = false;

	if (bytes == NULL) {
		VOE_BASE_ERROR("audio", "cannot read sound %s", clip->path);
	} else if (!voe_assets_wav_decode(bytes, size, mixer->scratch, &sound, NULL)) {
		VOE_BASE_ERROR("audio", "cannot decode sound %s", clip->path);
	} else if (sound.frames * VOE_PLATFORM_SOUND_RATE / sound.rate >
		   (uint64_t)VOE_AUDIO_CLIP_SECONDS * VOE_PLATFORM_SOUND_RATE) {
		VOE_BASE_ERROR("audio", "sound %s is longer than %d seconds",
			       clip->path, VOE_AUDIO_CLIP_SECONDS);
	} else {
		const uint64_t frames = sound.frames * VOE_PLATFORM_SOUND_RATE / sound.rate;
		float *samples = voe_base_arena_push(
			mixer->keep, (size_t)frames * MIXER_CHANNELS * sizeof(float));

		// Linear interpolation between the two source frames either side of
		// where an output frame falls; the last frame pairs with itself.
		for (uint64_t i = 0; i < frames; i++) {
			const double at = (double)i * sound.rate / VOE_PLATFORM_SOUND_RATE;
			const uint64_t before = (uint64_t)at;
			const uint64_t after = before + 1 < sound.frames ? before + 1 : before;
			const float weight = (float)(at - (double)before);

			for (uint32_t c = 0; c < MIXER_CHANNELS; c++) {
				const float a = source_sample(&sound, before, c);
				const float b = source_sample(&sound, after, c);

				samples[i * MIXER_CHANNELS + c] = a + (b - a) * weight;
			}
		}
		clip->frames = frames;
		clip->samples = samples;
		loaded = true;
	}
	voe_base_arena_rewind(mixer->scratch, mark);
	return loaded;
}

// The loaded clip for path, loading it on its first ask. MIXER_NO_CLIP for a
// path that failed, now or before, and for one past VOE_AUDIO_CLIPS.
static uint32_t find_clip(voe_audio_mixer *mixer, const char *path)
{
	for (uint32_t i = 0; i < mixer->clip_count; i++) {
		if (strcmp(mixer->clips[i].path, path) == 0)
			return mixer->clips[i].failed ? MIXER_NO_CLIP : i;
	}
	if (mixer->clip_count == VOE_AUDIO_CLIPS) {
		if (!mixer->full_reported)
			VOE_BASE_ERROR("audio", "more than %d sounds; %s and later ones are silent",
				       VOE_AUDIO_CLIPS, path);
		mixer->full_reported = true;
		return MIXER_NO_CLIP;
	}
	const uint32_t index = mixer->clip_count++;
	struct mixer_clip *clip = &mixer->clips[index];

	clip->path = copy_text(mixer->keep, path);
	clip->failed = !load_clip(mixer, clip);
	return clip->failed ? MIXER_NO_CLIP : index;
}

// value within [low, high]; NaN, as from a broken component, gives low.
static float clamp(float value, float low, float high)
{
	return value > high ? high : value >= low ? value : low;
}

// A free slot, else the oldest voice that does not loop; VOE_AUDIO_VOICES
// when every voice loops.
static uint32_t take_slot(const voe_audio_mixer *mixer)
{
	uint32_t oldest = VOE_AUDIO_VOICES;

	for (uint32_t i = 0; i < VOE_AUDIO_VOICES; i++) {
		const struct mixer_voice *voice = &mixer->voices[i];

		if (!voice->busy)
			return i;
		if (!voice->loop && (oldest == VOE_AUDIO_VOICES ||
				     voice->started < mixer->voices[oldest].started))
			oldest = i;
	}
	return oldest;
}

// The slot a handle names while its voice plays; VOE_AUDIO_VOICES when stale.
static uint32_t find_slot(const voe_audio_mixer *mixer, voe_audio_voice handle)
{
	const uint32_t slot = handle.id & ((1u << MIXER_SLOT_BITS) - 1u);

	if (handle.id == 0 || slot >= VOE_AUDIO_VOICES)
		return VOE_AUDIO_VOICES;
	const struct mixer_voice *voice = &mixer->voices[slot];

	if (!voice->busy || voice->generation != handle.id >> MIXER_SLOT_BITS)
		return VOE_AUDIO_VOICES;
	return slot;
}

voe_audio_voice voe_audio_mixer_start(voe_audio_mixer *mixer, voe_audio_start start)
{
	VOE_BASE_ASSERT(mixer != NULL && start.path != NULL, "start needs a mixer and a path");
	const voe_audio_voice none = { 0 };

	if (start.path[0] == '\0')
		return none;
	const uint32_t clip = find_clip(mixer, start.path);

	if (clip == MIXER_NO_CLIP || mixer->clips[clip].frames == 0)
		return none;
	const uint32_t slot = take_slot(mixer);

	if (slot == VOE_AUDIO_VOICES)
		return none;
	struct mixer_voice *voice = &mixer->voices[slot];
	const uint32_t next = (voice->generation + 1u) & MIXER_GENERATION_MASK;
	const uint32_t generation = next != 0 ? next : 1u;
	const float gain = clamp(start.volume, 0.0f, MIXER_VOLUME_MAX);

	*voice = (struct mixer_voice){
		.busy = true, .loop = start.loop, .held = start.held, .touched = true,
		.clip = clip, .generation = generation, .position = 0.0,
		.pitch = clamp(start.pitch, MIXER_PITCH_MIN, MIXER_PITCH_MAX),
		.gain = gain, .gain_from = gain, .started = mixer->plays++,
	};
	return (voe_audio_voice){ generation << MIXER_SLOT_BITS | slot };
}

void voe_audio_mixer_play(voe_audio_mixer *mixer, const char *path)
{
	VOE_BASE_ASSERT(mixer != NULL && path != NULL, "play needs a mixer and a path");
	(void)voe_audio_mixer_start(mixer, (voe_audio_start){
		.path = path, .volume = 1.0f, .pitch = 1.0f });
}

void voe_audio_mixer_stop(voe_audio_mixer *mixer, voe_audio_voice voice)
{
	VOE_BASE_ASSERT(mixer != NULL, "stop needs a mixer");
	const uint32_t slot = find_slot(mixer, voice);

	if (slot != VOE_AUDIO_VOICES)
		mixer->voices[slot].busy = false;
}

bool voe_audio_mixer_playing(const voe_audio_mixer *mixer, voe_audio_voice voice)
{
	VOE_BASE_ASSERT(mixer != NULL, "playing needs a mixer");
	return find_slot(mixer, voice) != VOE_AUDIO_VOICES;
}

void voe_audio_mixer_tune(voe_audio_mixer *mixer, voe_audio_voice voice,
			  float volume, float pitch)
{
	VOE_BASE_ASSERT(mixer != NULL, "tune needs a mixer");
	const uint32_t slot = find_slot(mixer, voice);

	if (slot == VOE_AUDIO_VOICES)
		return;
	struct mixer_voice *tuned = &mixer->voices[slot];

	tuned->gain = clamp(volume, 0.0f, MIXER_VOLUME_MAX);
	tuned->pitch = clamp(pitch, MIXER_PITCH_MIN, MIXER_PITCH_MAX);
	tuned->touched = true;
}

void voe_audio_mixer_sweep(voe_audio_mixer *mixer)
{
	VOE_BASE_ASSERT(mixer != NULL, "sweep needs a mixer");
	for (uint32_t i = 0; i < VOE_AUDIO_VOICES; i++) {
		struct mixer_voice *voice = &mixer->voices[i];

		if (voice->held && !voice->touched)
			voice->busy = false;
		voice->touched = false;
	}
}

void voe_audio_mixer_mix(voe_audio_mixer *mixer, float *out, uint32_t frames)
{
	VOE_BASE_ASSERT(mixer != NULL && out != NULL, "mix needs a mixer and somewhere to write");
	const size_t count = (size_t)frames * MIXER_CHANNELS;

	memset(out, 0, count * sizeof(float));
	for (uint32_t v = 0; v < VOE_AUDIO_VOICES; v++) {
		struct mixer_voice *voice = &mixer->voices[v];

		if (voice->busy)
			voe_audio_voice_sum(voice, &mixer->clips[voice->clip], out, frames);
	}
	for (size_t i = 0; i < count; i++)
		out[i] = out[i] > 1.0f ? 1.0f : out[i] < -1.0f ? -1.0f : out[i];
}

bool voe_audio_mixer_pump(voe_audio_mixer *mixer, voe_platform_sound *device)
{
	VOE_BASE_ASSERT(mixer != NULL && device != NULL, "pump needs a mixer and a device");
	const uint32_t room = voe_platform_sound_room(device);

	VOE_BASE_ASSERT(room <= VOE_PLATFORM_SOUND_QUEUE, "the device promises at most its queue");
	if (room == 0)
		return true;
	voe_audio_mixer_mix(mixer, mixer->pumped, room);
	return voe_platform_sound_write(device, mixer->pumped, room);
}
