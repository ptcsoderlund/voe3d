// The Linux half of platform/sound.h: ALSA's "default" device, opened through
// voe_platform_library_new("libasound.so.2") and never linked (ADR-0265).
//
// NO ALSA HEADER. The nine calls used are resolved by name, and their
// prototypes and the four constants are declared below from ALSA's stable ABI,
// so building needs no libasound development package. snd_pcm_open's `**`
// is ALSA's shape at the boundary, kept as it is.
//
// snd_pcm_set_params does the setup in one call: float, interleaved, stereo,
// 48 kHz, soft resampling on so any hardware rate works, and a 100 ms buffer.
// The device is non-blocking; room is min(avail, QUEUE - queued) so the queue
// stays at 50 ms however large the buffer turned out.
//
// snd_pcm_start IS CALLED BY HAND. set_params sets the start threshold to the
// whole buffer, which a queue held to 50 ms of a 100 ms buffer never reaches,
// so the stream is started after the first write and again after every
// recovery. A negative avail or write goes through snd_pcm_recover, silently;
// a recovery that fails is the device gone.
#include <platform/sound.h>

#include <platform/library.h>

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>

#define SND_PCM_STREAM_PLAYBACK 0
#define SND_PCM_NONBLOCK 0x1
#define SND_PCM_FORMAT_FLOAT_LE 14
#define SND_PCM_ACCESS_RW_INTERLEAVED 3
#define LATENCY_MICROSECONDS 100000u

typedef struct snd_pcm snd_pcm;
typedef int (*pcm_open_fn)(snd_pcm **pcm, const char *name, int stream, int mode);
typedef int (*pcm_set_params_fn)(snd_pcm *pcm, int format, int access, unsigned channels,
				 unsigned rate, int soft_resample, unsigned latency);
typedef int (*pcm_get_params_fn)(snd_pcm *pcm, unsigned long *buffer, unsigned long *period);
typedef long (*pcm_avail_update_fn)(snd_pcm *pcm);
typedef long (*pcm_writei_fn)(snd_pcm *pcm, const void *frames, unsigned long count);
typedef int (*pcm_recover_fn)(snd_pcm *pcm, int error, int silent);
typedef int (*pcm_start_fn)(snd_pcm *pcm);
typedef int (*pcm_close_fn)(snd_pcm *pcm);
typedef const char *(*strerror_fn)(int error);

struct voe_platform_sound {
	voe_platform_library *library;
	snd_pcm *pcm;
	pcm_avail_update_fn avail_update;
	pcm_writei_fn writei;
	pcm_recover_fn recover;
	pcm_start_fn start;
	pcm_close_fn close;
	strerror_fn strerror;
	long buffer;
	uint32_t room;
	bool started;
};

static bool resolve(struct voe_platform_sound *sound, pcm_open_fn *pcm_open,
		    pcm_set_params_fn *set_params, pcm_get_params_fn *get_params)
{
	voe_platform_library *library = sound->library;

	*pcm_open = (pcm_open_fn)voe_platform_library_symbol(library, "snd_pcm_open");
	*set_params = (pcm_set_params_fn)voe_platform_library_symbol(library, "snd_pcm_set_params");
	*get_params = (pcm_get_params_fn)voe_platform_library_symbol(library, "snd_pcm_get_params");
	sound->avail_update =
		(pcm_avail_update_fn)voe_platform_library_symbol(library, "snd_pcm_avail_update");
	sound->writei = (pcm_writei_fn)voe_platform_library_symbol(library, "snd_pcm_writei");
	sound->recover = (pcm_recover_fn)voe_platform_library_symbol(library, "snd_pcm_recover");
	sound->start = (pcm_start_fn)voe_platform_library_symbol(library, "snd_pcm_start");
	sound->close = (pcm_close_fn)voe_platform_library_symbol(library, "snd_pcm_close");
	sound->strerror = (strerror_fn)voe_platform_library_symbol(library, "snd_strerror");
	return *pcm_open != NULL && *set_params != NULL && *get_params != NULL &&
	       sound->avail_update != NULL && sound->writei != NULL && sound->recover != NULL &&
	       sound->start != NULL && sound->close != NULL && sound->strerror != NULL;
}

// Opens and configures the PCM; on failure reports which call and leaves
// sound->pcm NULL.
static bool open_default_device(struct voe_platform_sound *sound, pcm_open_fn pcm_open,
				pcm_set_params_fn set_params, pcm_get_params_fn get_params)
{
	unsigned long buffer = 0;
	unsigned long period = 0;
	int error;

	error = pcm_open(&sound->pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (error < 0) {
		sound->pcm = NULL;
		VOE_BASE_ERROR("platform", "no sound: snd_pcm_open: %s", sound->strerror(error));
		return false;
	}
	error = set_params(sound->pcm, SND_PCM_FORMAT_FLOAT_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
			   VOE_PLATFORM_SOUND_CHANNELS, VOE_PLATFORM_SOUND_RATE, 1,
			   LATENCY_MICROSECONDS);
	if (error < 0) {
		VOE_BASE_ERROR("platform", "no sound: snd_pcm_set_params: %s",
			       sound->strerror(error));
	} else {
		error = get_params(sound->pcm, &buffer, &period);
		if (error < 0)
			VOE_BASE_ERROR("platform", "no sound: snd_pcm_get_params: %s",
				       sound->strerror(error));
	}
	if (error < 0) {
		sound->close(sound->pcm);
		sound->pcm = NULL;
		return false;
	}
	sound->buffer = (long)buffer;
	return true;
}

voe_platform_sound *voe_platform_sound_new(void)
{
	struct voe_platform_sound *sound;
	pcm_open_fn pcm_open;
	pcm_set_params_fn set_params;
	pcm_get_params_fn get_params;

	sound = calloc(1, sizeof(*sound));
	if (sound == NULL) {
		VOE_BASE_ERROR("platform", "no sound: out of memory");
		return NULL;
	}
	sound->library = voe_platform_library_new("libasound.so.2");
	if (sound->library == NULL)
		goto free_sound;
	if (!resolve(sound, &pcm_open, &set_params, &get_params)) {
		VOE_BASE_ERROR("platform", "no sound: libasound.so.2 lacks a snd_pcm call");
		goto close_library;
	}
	if (!open_default_device(sound, pcm_open, set_params, get_params))
		goto close_library;
	VOE_BASE_DEBUG_ASSERT(sound->pcm != NULL, "an open device has a PCM");
	return sound;

close_library:
	voe_platform_library_destroy(sound->library);
free_sound:
	free(sound);
	return NULL;
}

void voe_platform_sound_destroy(voe_platform_sound *sound)
{
	if (sound == NULL)
		return;
	VOE_BASE_DEBUG_ASSERT(sound->pcm != NULL, "a sound without a PCM");

	sound->close(sound->pcm);
	voe_platform_library_destroy(sound->library);
	free(sound);
}

uint32_t voe_platform_sound_room(voe_platform_sound *sound)
{
	long avail;
	long room;

	VOE_BASE_DEBUG_ASSERT(sound != NULL, "room on a NULL sound");

	avail = sound->avail_update(sound->pcm);
	if (avail < 0) {
		sound->started = false;
		if (sound->recover(sound->pcm, (int)avail, 1) < 0)
			avail = 0;
		else
			avail = sound->avail_update(sound->pcm);
	}
	room = VOE_PLATFORM_SOUND_QUEUE - (sound->buffer - avail);
	if (avail < room)
		room = avail;
	sound->room = room > 0 ? (uint32_t)room : 0;
	VOE_BASE_DEBUG_ASSERT(sound->room <= VOE_PLATFORM_SOUND_QUEUE, "room past the queue");
	return sound->room;
}

bool voe_platform_sound_write(voe_platform_sound *sound, const float *frames, uint32_t count)
{
	long written;
	int error;

	VOE_BASE_DEBUG_ASSERT(sound != NULL, "writing to a NULL sound");
	VOE_BASE_ASSERT(count <= sound->room, "writing more frames than the last room");
	VOE_BASE_DEBUG_ASSERT(count == 0 || frames != NULL, "writing frames from NULL");

	sound->room -= count;
	if (count == 0)
		return true;
	written = sound->writei(sound->pcm, frames, count);
	if (written < 0) {
		sound->started = false;
		error = sound->recover(sound->pcm, (int)written, 1);
		if (error < 0) {
			VOE_BASE_ERROR("platform", "sound device lost: %s", sound->strerror(error));
			return false;
		}
		return true;
	}
	if (!sound->started) {
		// Failing means it already started on its own threshold; it runs.
		(void)sound->start(sound->pcm);
		sound->started = true;
	}
	return true;
}
