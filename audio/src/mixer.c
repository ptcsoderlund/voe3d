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
#include <audio/mixer.h>

#include <assets/sound.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

#include <string.h>

#define MIXER_CHANNELS VOE_PLATFORM_SOUND_CHANNELS
#define MIXER_NO_CLIP UINT32_MAX
#define MIXER_SCRATCH_BLOCK (1u << 20)
#define MIXER_KEEP_BLOCK (1u << 20)

struct mixer_clip {
	const char *path;
	bool failed;
	uint64_t frames;
	const float *samples;
};

struct mixer_voice {
	bool busy;
	uint32_t clip;
	uint64_t position;
	// The play count when this voice started; the smallest is the oldest.
	uint64_t started;
};

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

void voe_audio_mixer_play(voe_audio_mixer *mixer, const char *path)
{
	VOE_BASE_ASSERT(mixer != NULL && path != NULL, "play needs a mixer and a path");
	if (path[0] == '\0')
		return;
	const uint32_t clip = find_clip(mixer, path);

	if (clip == MIXER_NO_CLIP || mixer->clips[clip].frames == 0)
		return;
	struct mixer_voice *voice = &mixer->voices[0];

	for (uint32_t i = 0; i < VOE_AUDIO_VOICES; i++) {
		struct mixer_voice *candidate = &mixer->voices[i];

		if (!candidate->busy) {
			voice = candidate;
			break;
		}
		if (candidate->started < voice->started)
			voice = candidate;
	}
	*voice = (struct mixer_voice){
		.busy = true, .clip = clip, .position = 0, .started = mixer->plays++,
	};
}

void voe_audio_mixer_mix(voe_audio_mixer *mixer, float *out, uint32_t frames)
{
	VOE_BASE_ASSERT(mixer != NULL && out != NULL, "mix needs a mixer and somewhere to write");
	const size_t count = (size_t)frames * MIXER_CHANNELS;

	memset(out, 0, count * sizeof(float));
	for (uint32_t v = 0; v < VOE_AUDIO_VOICES; v++) {
		struct mixer_voice *voice = &mixer->voices[v];

		if (!voice->busy)
			continue;
		const struct mixer_clip *clip = &mixer->clips[voice->clip];
		const uint64_t left = clip->frames - voice->position;
		const uint64_t take = left < frames ? left : frames;
		const float *from = clip->samples + voice->position * MIXER_CHANNELS;

		for (size_t i = 0; i < take * MIXER_CHANNELS; i++)
			out[i] += from[i];
		voice->position += take;
		voice->busy = voice->position < clip->frames;
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
