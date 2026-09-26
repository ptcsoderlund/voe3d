// The mixer, end to end from files: WAVs written into a folder this test makes
// under its working directory, played by path, and the mixed frames compared
// with what the clips hold. It checks the conversion (a 24 kHz mono clip doubles
// its length and fills both sides), overlap without cutting, the voice limit,
// that a missing file is reported once and is silence, that "" is quiet, and the
// clamp at 1.
//
// Every clip is mono 16-bit, whose samples are exact in float, so the checks
// can compare sums with a tight tolerance. The voice count is read through the
// mix: a constant clip played k times sums to k times its value.
#include <audio/mixer.h>

#include <base/report.h>
#include <platform/file.h>
#include <platform/folder.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define FOLDER "audio_mixer_test"
#define MAX_FRAMES 1024
#define TOLERANCE 1e-6f

static float out[MAX_FRAMES * VOE_PLATFORM_SOUND_CHANNELS];

static void put_le(uint8_t *at, uint32_t value, uint32_t bytes)
{
	for (uint32_t i = 0; i < bytes; i++)
		at[i] = (uint8_t)(value >> (8 * i));
}

// A mono 16-bit PCM WAV of frames samples, each first + i * step, at name.
static void write_wav(const char *name, uint32_t rate, uint32_t frames,
		      int16_t first, int16_t step)
{
	static uint8_t bytes[44 + 2 * MAX_FRAMES];
	const uint32_t data = frames * 2;
	char path[256];

	memcpy(bytes, "RIFF", 4);
	put_le(bytes + 4, 36 + data, 4);
	memcpy(bytes + 8, "WAVEfmt ", 8);
	put_le(bytes + 16, 16, 4);
	put_le(bytes + 20, 1, 2);
	put_le(bytes + 22, 1, 2);
	put_le(bytes + 24, rate, 4);
	put_le(bytes + 28, rate * 2, 4);
	put_le(bytes + 32, 2, 2);
	put_le(bytes + 34, 16, 2);
	memcpy(bytes + 36, "data", 4);
	put_le(bytes + 40, data, 4);
	for (uint32_t i = 0; i < frames; i++)
		put_le(bytes + 44 + 2 * i, (uint16_t)(int16_t)(first + (int)i * step), 2);
	snprintf(path, sizeof(path), "%s/%s", FOLDER, name);
	VOE_TEST_CHECK(voe_platform_file_write(path, bytes, 44 + data, NULL));
}

static float left(uint32_t frame) { return out[frame * 2]; }
static float right(uint32_t frame) { return out[frame * 2 + 1]; }

static void converts_mono_24k_to_stereo_48k(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("ramp.wav", 24000, 100, 1000, 10);
	voe_audio_mixer_play(mixer, "ramp.wav");
	voe_audio_mixer_mix(mixer, out, 210);
	for (uint32_t f = 0; f < 200; f++)
		VOE_TEST_CHECK_FLOAT(left(f), right(f), 0.0f);
	VOE_TEST_CHECK_FLOAT(left(0), 1000.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(20), 1100.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(21), 1105.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK(left(199) > 0.0f);
	VOE_TEST_CHECK_FLOAT(left(200), 0.0f, 0.0f);
	voe_audio_mixer_mix(mixer, out, 10);
	VOE_TEST_CHECK_FLOAT(left(0), 0.0f, 0.0f);
	voe_audio_mixer_destroy(mixer);
}

static void overlapping_plays_sum(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("quarter.wav", 48000, 300, 8192, 0);
	voe_audio_mixer_play(mixer, "quarter.wav");
	voe_audio_mixer_mix(mixer, out, 100);
	voe_audio_mixer_play(mixer, "quarter.wav");
	voe_audio_mixer_mix(mixer, out, 300);
	VOE_TEST_CHECK_FLOAT(left(0), 0.5f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(right(199), 0.5f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(200), 0.25f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(299), 0.25f, TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static void seventeen_plays_leave_sixteen_voices(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("quiet.wav", 48000, 50, 256, 0);
	for (int i = 0; i < 17; i++)
		voe_audio_mixer_play(mixer, "quiet.wav");
	voe_audio_mixer_mix(mixer, out, 1);
	VOE_TEST_CHECK_FLOAT(left(0), 16.0f * 256.0f / 32768.0f, TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static void missing_file_reports_once_and_is_silent(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	voe_base_report_error_clear();
	voe_audio_mixer_play(mixer, "missing.wav");
	VOE_TEST_CHECK(voe_base_report_error_first() != NULL);
	voe_base_report_error_clear();
	voe_audio_mixer_play(mixer, "missing.wav");
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	voe_audio_mixer_mix(mixer, out, 10);
	for (uint32_t f = 0; f < 10; f++)
		VOE_TEST_CHECK_FLOAT(left(f) + right(f), 0.0f, 0.0f);
	voe_base_report_error_clear();
	voe_audio_mixer_play(mixer, "");
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	voe_audio_mixer_destroy(mixer);
}

static void loud_clip_twice_clamps(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("loud.wav", 48000, 20, 24576, 0);
	voe_audio_mixer_play(mixer, "loud.wav");
	voe_audio_mixer_play(mixer, "loud.wav");
	voe_audio_mixer_mix(mixer, out, 20);
	VOE_TEST_CHECK_FLOAT(left(0), 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(right(19), 1.0f, 0.0f);
	voe_audio_mixer_destroy(mixer);
}

int main(void)
{
	voe_base_error error = VOE_BASE_OK;

	// A folder left by an earlier run is taken as it is.
	if (!voe_platform_folder_create(FOLDER, &error))
		VOE_TEST_CHECK(error == VOE_BASE_ERROR_REFUSED);
	converts_mono_24k_to_stereo_48k();
	overlapping_plays_sum();
	seventeen_plays_leave_sixteen_voices();
	missing_file_reports_once_and_is_silent();
	loud_clip_twice_clamps();
	return voe_test_result();
}
