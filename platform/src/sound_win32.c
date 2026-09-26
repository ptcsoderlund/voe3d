// The Windows half of platform/sound.h: WASAPI in shared mode on the default
// render endpoint, through COM in C (COBJMACROS). Written, not verified: no
// Windows machine has run it (ADR-0130).
//
// ole32.dll IS LOADED AT RUN TIME with voe_platform_library_new for
// CoInitializeEx and CoCreateInstance, and the four GUIDs are defined here
// rather than taken from uuid.lib, so the build links nothing new (ADR-0266).
// ole32 stays open until destroy, below every COM object made through it.
// COM is left initialised on the calling thread; a thread already in another
// apartment (RPC_E_CHANGED_MODE) still works for this, so that is not a failure.
//
// The stream is 48 kHz stereo float (WAVE_FORMAT_IEEE_FLOAT) with
// AUTOCONVERTPCM and SRC_DEFAULT_QUALITY, so the audio engine converts to its
// mix format, and a 100 ms buffer, started at once. Room is the buffer's free
// part capped so padding plus room never passes VOE_PLATFORM_SOUND_QUEUE; a
// failed GetCurrentPadding (the device invalidated) marks the sound lost, and
// the next write reports it and returns false.
#define COBJMACROS
#include <platform/sound.h>

#include <platform/library.h>

#include <base/assert.h>
#include <base/report.h>

#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <stdlib.h>
#include <string.h>

#define BUFFER_100NS 1000000

static const GUID clsid_device_enumerator = {
	0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
static const GUID iid_device_enumerator = {
	0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
static const GUID iid_audio_client = {
	0x1CB9AD4C, 0xDBFA, 0x4C32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
static const GUID iid_render_client = {
	0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};

typedef HRESULT(WINAPI *co_initialize_ex_fn)(LPVOID reserved, DWORD model);
typedef HRESULT(WINAPI *co_create_instance_fn)(REFCLSID clsid, LPUNKNOWN outer, DWORD context,
					       REFIID iid, LPVOID *object);

struct voe_platform_sound {
	voe_platform_library *ole32;
	IMMDeviceEnumerator *enumerator;
	IMMDevice *device;
	IAudioClient *client;
	IAudioRenderClient *render;
	UINT32 buffer;
	uint32_t room;
	bool lost;
};

// Everything from the enumerator to a started stream; reports the failing call.
static bool open_default_device(struct voe_platform_sound *sound, co_create_instance_fn create)
{
	WAVEFORMATEX format = {0};
	const char *call;
	HRESULT result;

	format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	format.nChannels = VOE_PLATFORM_SOUND_CHANNELS;
	format.nSamplesPerSec = VOE_PLATFORM_SOUND_RATE;
	format.wBitsPerSample = 32;
	format.nBlockAlign = VOE_PLATFORM_SOUND_CHANNELS * sizeof(float);
	format.nAvgBytesPerSec = VOE_PLATFORM_SOUND_RATE * format.nBlockAlign;

	call = "CoCreateInstance";
	result = create(&clsid_device_enumerator, NULL, CLSCTX_ALL, &iid_device_enumerator,
			(LPVOID *)&sound->enumerator);
	if (FAILED(result))
		goto report;
	call = "GetDefaultAudioEndpoint";
	result = IMMDeviceEnumerator_GetDefaultAudioEndpoint(sound->enumerator, eRender, eConsole,
							     &sound->device);
	if (FAILED(result))
		goto report;
	call = "IMMDevice::Activate";
	result = IMMDevice_Activate(sound->device, &iid_audio_client, CLSCTX_ALL, NULL,
				    (LPVOID *)&sound->client);
	if (FAILED(result))
		goto report;
	call = "IAudioClient::Initialize";
	result = IAudioClient_Initialize(sound->client, AUDCLNT_SHAREMODE_SHARED,
					 AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
						 AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
					 BUFFER_100NS, 0, &format, NULL);
	if (FAILED(result))
		goto report;
	call = "IAudioClient::GetBufferSize";
	result = IAudioClient_GetBufferSize(sound->client, &sound->buffer);
	if (FAILED(result))
		goto report;
	call = "IAudioClient::GetService";
	result = IAudioClient_GetService(sound->client, &iid_render_client,
					 (LPVOID *)&sound->render);
	if (FAILED(result))
		goto report;
	call = "IAudioClient::Start";
	result = IAudioClient_Start(sound->client);
	if (FAILED(result))
		goto report;
	return true;

report:
	VOE_BASE_ERROR("platform", "no sound: %s failed (0x%08lx)", call, (unsigned long)result);
	return false;
}

// Releases whatever open_default_device got to, in reverse, then ole32.
static void release_all(struct voe_platform_sound *sound)
{
	if (sound->render != NULL)
		IAudioRenderClient_Release(sound->render);
	if (sound->client != NULL) {
		IAudioClient_Stop(sound->client);
		IAudioClient_Release(sound->client);
	}
	if (sound->device != NULL)
		IMMDevice_Release(sound->device);
	if (sound->enumerator != NULL)
		IMMDeviceEnumerator_Release(sound->enumerator);
	voe_platform_library_destroy(sound->ole32);
	free(sound);
}

voe_platform_sound *voe_platform_sound_new(void)
{
	struct voe_platform_sound *sound;
	co_initialize_ex_fn initialize;
	co_create_instance_fn create;
	HRESULT result;

	sound = calloc(1, sizeof(*sound));
	if (sound == NULL) {
		VOE_BASE_ERROR("platform", "no sound: out of memory");
		return NULL;
	}
	sound->ole32 = voe_platform_library_new("ole32.dll");
	if (sound->ole32 == NULL) {
		free(sound);
		return NULL;
	}
	initialize = (co_initialize_ex_fn)voe_platform_library_symbol(sound->ole32, "CoInitializeEx");
	create = (co_create_instance_fn)voe_platform_library_symbol(sound->ole32, "CoCreateInstance");
	if (initialize == NULL || create == NULL) {
		VOE_BASE_ERROR("platform", "no sound: ole32.dll lacks CoInitializeEx or CoCreateInstance");
		release_all(sound);
		return NULL;
	}
	result = initialize(NULL, COINIT_MULTITHREADED);
	if (FAILED(result) && result != RPC_E_CHANGED_MODE) {
		VOE_BASE_ERROR("platform", "no sound: CoInitializeEx failed (0x%08lx)",
			       (unsigned long)result);
		release_all(sound);
		return NULL;
	}
	if (!open_default_device(sound, create)) {
		release_all(sound);
		return NULL;
	}
	VOE_BASE_DEBUG_ASSERT(sound->render != NULL, "an open device has a render client");
	return sound;
}

void voe_platform_sound_destroy(voe_platform_sound *sound)
{
	if (sound == NULL)
		return;
	VOE_BASE_DEBUG_ASSERT(sound->render != NULL, "a sound without a render client");

	release_all(sound);
}

uint32_t voe_platform_sound_room(voe_platform_sound *sound)
{
	UINT32 padding;
	uint32_t room;

	VOE_BASE_DEBUG_ASSERT(sound != NULL, "room on a NULL sound");

	sound->room = 0;
	if (sound->lost)
		return 0;
	if (FAILED(IAudioClient_GetCurrentPadding(sound->client, &padding))) {
		sound->lost = true;
		return 0;
	}
	room = sound->buffer - padding;
	if (padding >= VOE_PLATFORM_SOUND_QUEUE)
		room = 0;
	else if (room > VOE_PLATFORM_SOUND_QUEUE - padding)
		room = VOE_PLATFORM_SOUND_QUEUE - padding;
	sound->room = room;
	VOE_BASE_DEBUG_ASSERT(sound->room <= VOE_PLATFORM_SOUND_QUEUE, "room past the queue");
	return sound->room;
}

bool voe_platform_sound_write(voe_platform_sound *sound, const float *frames, uint32_t count)
{
	BYTE *data;
	HRESULT result;

	VOE_BASE_DEBUG_ASSERT(sound != NULL, "writing to a NULL sound");
	VOE_BASE_ASSERT(count <= sound->room, "writing more frames than the last room");
	VOE_BASE_DEBUG_ASSERT(count == 0 || frames != NULL, "writing frames from NULL");

	sound->room -= count;
	result = S_OK;
	if (!sound->lost && count > 0) {
		result = IAudioRenderClient_GetBuffer(sound->render, count, &data);
		if (SUCCEEDED(result)) {
			memcpy(data, frames, (size_t)count * VOE_PLATFORM_SOUND_CHANNELS * sizeof(float));
			result = IAudioRenderClient_ReleaseBuffer(sound->render, count, 0);
		}
	}
	if (sound->lost || FAILED(result)) {
		sound->lost = true;
		VOE_BASE_ERROR("platform", "sound device lost (0x%08lx)", (unsigned long)result);
		return false;
	}
	return true;
}
