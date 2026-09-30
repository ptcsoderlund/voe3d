// The mixer, end to end from files: WAVs written into a folder this test makes
// under its working directory, played by path, and the mixed frames compared
// with what the clips hold. It checks the conversion (a 24 kHz mono clip doubles
// its length and fills both sides), overlap without cutting, the voice limit,
// that a missing file is reported once and is silence, that "" is quiet, the
// clamp at 1, and the voice handle: a seamless loop, pitch as read rate, a
// ramped volume, stop and stale handles, the steal rule, the held sweep, and a
// voice placed left of a listener heard louder on the left until it is cleared.
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

static void thirty_three_plays_leave_thirty_two_voices(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("quiet.wav", 48000, 50, 256, 0);
	for (int i = 0; i < VOE_AUDIO_VOICES + 1; i++)
		voe_audio_mixer_play(mixer, "quiet.wav");
	voe_audio_mixer_mix(mixer, out, 1);
	VOE_TEST_CHECK_FLOAT(left(0), VOE_AUDIO_VOICES * 256.0f / 32768.0f, TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static voe_audio_voice start(voe_audio_mixer *mixer, const char *path, bool loop,
			     bool held, float pitch)
{
	return voe_audio_mixer_start(mixer, (voe_audio_start){
		.path = path, .loop = loop, .held = held, .volume = 1.0f, .pitch = pitch });
}

static void loop_wraps_without_a_gap(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("short.wav", 48000, 10, 1000, 100);
	VOE_TEST_CHECK(start(mixer, "short.wav", true, false, 1.0f).id != 0);
	voe_audio_mixer_mix(mixer, out, 25);
	for (uint32_t f = 0; f < 25; f++)
		VOE_TEST_CHECK_FLOAT(left(f), (1000.0f + 100.0f * (float)(f % 10)) / 32768.0f,
				     TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static void pitch_two_plays_in_half_the_frames(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("long.wav", 48000, 100, 1000, 10);
	const voe_audio_voice voice = start(mixer, "long.wav", false, false, 2.0f);

	voe_audio_mixer_mix(mixer, out, 60);
	VOE_TEST_CHECK_FLOAT(left(0), 1000.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(1), 1020.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(49), 1980.0f / 32768.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(50), 0.0f, 0.0f);
	VOE_TEST_CHECK(!voe_audio_mixer_playing(mixer, voice));
	voe_audio_mixer_destroy(mixer);
}

static void volume_half_halves_after_the_ramp(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("steady.wav", 48000, 300, 8192, 0);
	const voe_audio_voice voice = start(mixer, "steady.wav", false, false, 1.0f);

	voe_audio_mixer_mix(mixer, out, 10);
	voe_audio_mixer_tune(mixer, voice, 0.5f, 1.0f);
	voe_audio_mixer_mix(mixer, out, 10);
	VOE_TEST_CHECK(left(0) < 0.25f && left(0) > 0.125f);
	voe_audio_mixer_mix(mixer, out, 10);
	VOE_TEST_CHECK_FLOAT(left(0), 0.125f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(right(9), 0.125f, TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static void stop_silences_and_stale_is_a_no_op(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("steady.wav", 48000, 300, 8192, 0);
	const voe_audio_voice old = start(mixer, "steady.wav", false, false, 1.0f);

	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, old));
	voe_audio_mixer_stop(mixer, old);
	VOE_TEST_CHECK(!voe_audio_mixer_playing(mixer, old));
	voe_audio_mixer_mix(mixer, out, 5);
	VOE_TEST_CHECK_FLOAT(left(0), 0.0f, 0.0f);
	// The new voice takes the same slot; the old handle must not reach it.
	const voe_audio_voice fresh = start(mixer, "steady.wav", false, false, 1.0f);

	VOE_TEST_CHECK(fresh.id != old.id);
	voe_audio_mixer_stop(mixer, old);
	voe_audio_mixer_tune(mixer, old, 0.0f, 1.0f);
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, fresh));
	voe_audio_mixer_mix(mixer, out, 5);
	VOE_TEST_CHECK_FLOAT(left(4), 0.25f, TOLERANCE);
	voe_audio_mixer_destroy(mixer);
}

static void all_looping_refuses_and_a_one_shot_takes_the_oldest(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);
	voe_audio_voice loops[VOE_AUDIO_VOICES];

	write_wav("steady.wav", 48000, 300, 8192, 0);
	for (uint32_t i = 0; i < VOE_AUDIO_VOICES; i++)
		loops[i] = start(mixer, "steady.wav", true, false, 1.0f);
	VOE_TEST_CHECK(start(mixer, "steady.wav", true, false, 1.0f).id == 0);
	VOE_TEST_CHECK(start(mixer, "steady.wav", false, false, 1.0f).id == 0);
	voe_audio_mixer_stop(mixer, loops[3]);
	voe_audio_mixer_stop(mixer, loops[7]);
	const voe_audio_voice older = start(mixer, "steady.wav", false, false, 1.0f);
	const voe_audio_voice newer = start(mixer, "steady.wav", false, false, 1.0f);
	const voe_audio_voice newest = start(mixer, "steady.wav", false, false, 1.0f);

	VOE_TEST_CHECK(newest.id != 0);
	VOE_TEST_CHECK(!voe_audio_mixer_playing(mixer, older));
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, newer));
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, loops[0]));
	voe_audio_mixer_destroy(mixer);
}

static void untuned_held_voice_stops_at_the_second_sweep(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);

	write_wav("steady.wav", 48000, 300, 8192, 0);
	const voe_audio_voice untuned = start(mixer, "steady.wav", true, true, 1.0f);
	const voe_audio_voice tuned = start(mixer, "steady.wav", true, true, 1.0f);

	voe_audio_mixer_sweep(mixer);
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, untuned));
	voe_audio_mixer_tune(mixer, tuned, 1.0f, 1.0f);
	voe_audio_mixer_sweep(mixer);
	VOE_TEST_CHECK(!voe_audio_mixer_playing(mixer, untuned));
	VOE_TEST_CHECK(voe_audio_mixer_playing(mixer, tuned));
	voe_audio_mixer_destroy(mixer);
}

static void placed_left_is_louder_left_until_the_listener_goes(void)
{
	voe_audio_mixer *mixer = voe_audio_mixer_new(FOLDER);
	const voe_audio_listener ear = voe_audio_listener_make(
		(voe_math_double3){ 0, 0, 0 }, (voe_math_float3){ 1, 0, 0 },
		(voe_math_float3){ 0, 0, -1 }, 1.0f);

	write_wav("steady.wav", 48000, 300, 8192, 0);
	voe_audio_mixer_listen(mixer, &ear);
	VOE_TEST_CHECK(voe_audio_mixer_play_at(mixer, "steady.wav",
					       (voe_math_double3){ -3, 0, -3 }).id != 0);
	voe_audio_mixer_mix(mixer, out, 10);
	VOE_TEST_CHECK(left(0) > right(0));
	VOE_TEST_CHECK(left(9) > right(9));
	voe_audio_mixer_listen(mixer, NULL);
	voe_audio_mixer_mix(mixer, out, 10);
	voe_audio_mixer_mix(mixer, out, 10);
	VOE_TEST_CHECK_FLOAT(left(0), right(0), TOLERANCE);
	VOE_TEST_CHECK_FLOAT(left(9), 0.25f, TOLERANCE);
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
	thirty_three_plays_leave_thirty_two_voices();
	missing_file_reports_once_and_is_silent();
	loud_clip_twice_clamps();
	loop_wraps_without_a_gap();
	pitch_two_plays_in_half_the_frames();
	volume_half_halves_after_the_ramp();
	stop_silences_and_stale_is_a_no_op();
	all_looping_refuses_and_a_one_shot_takes_the_oldest();
	untuned_held_voice_stops_at_the_second_sweep();
	placed_left_is_louder_left_until_the_listener_goes();
	return voe_test_result();
}
