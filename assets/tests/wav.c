// The WAV reader: files assembled here byte by byte, decoded, and compared with
// the samples that went in; then each refusal sound.h promises, with its
// category, and `sound` left as it was.
//
// THE FILES ARE BUILT RATHER THAN RECORDED because a WAV is a dozen fields in
// front of raw samples: writing them out keeps every byte under test visible,
// and one field changed is one case.
#include <assets/sound.h>
#include <base/arena.h>

#include <testing/test.h>

#include <string.h>

#define SCRATCH (1u << 16)

struct wav_file {
	uint8_t bytes[256];
	size_t size;
};

static void put(struct wav_file *file, const void *bytes, size_t count)
{
	VOE_TEST_CHECK(file->size + count <= sizeof(file->bytes));
	memcpy(file->bytes + file->size, bytes, count);
	file->size += count;
}

static void put16(struct wav_file *file, uint16_t value)
{
	const uint8_t bytes[2] = { (uint8_t)value, (uint8_t)(value >> 8) };

	put(file, bytes, sizeof(bytes));
}

static void put32(struct wav_file *file, uint32_t value)
{
	put16(file, (uint16_t)value);
	put16(file, (uint16_t)(value >> 16));
}

// "RIFF", a size patched in by finish(), and the form type.
static void start(struct wav_file *file, const char *form)
{
	file->size = 0;
	put(file, "RIFF", 4);
	put32(file, 0);
	put(file, form, 4);
}

static void finish(struct wav_file *file)
{
	const uint32_t riff = (uint32_t)file->size - 8;

	for (size_t i = 0; i < 4; i++)
		file->bytes[4 + i] = (uint8_t)(riff >> (8 * i));
}

static void put_fmt(struct wav_file *file, uint16_t tag, uint16_t channels,
		    uint32_t rate, uint16_t bits)
{
	const uint16_t align = (uint16_t)(channels * bits / 8);

	put(file, "fmt ", 4);
	put32(file, 16);
	put16(file, tag);
	put16(file, channels);
	put32(file, rate);
	put32(file, rate * align);
	put16(file, align);
	put16(file, bits);
}

static void put_data(struct wav_file *file, const void *samples, uint32_t size)
{
	put(file, "data", 4);
	put32(file, size);
	put(file, samples, size);
}

static voe_base_error decode(const struct wav_file *file, size_t size,
			     voe_base_arena *arena, voe_assets_sound *sound)
{
	voe_base_error error = VOE_BASE_OK;

	if (!voe_assets_wav_decode(file->bytes, size, arena, sound, &error))
		VOE_TEST_CHECK(error != VOE_BASE_OK);
	return error;
}

// Refused with `expected`, and `sound` still holds the marker it went in with.
static void refuses(const struct wav_file *file, size_t size,
		    voe_base_error expected)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_sound sound = { .rate = 7, .channels = 7, .frames = 7 };

	VOE_TEST_CHECK_INT((int)decode(file, size, arena, &sound), (int)expected);
	VOE_TEST_CHECK_INT((int)sound.rate, 7);
	VOE_TEST_CHECK_INT((int)sound.channels, 7);
	VOE_TEST_CHECK_INT((int)sound.frames, 7);
	VOE_TEST_CHECK(sound.samples == NULL);
	voe_base_arena_destroy(arena);
}

static void mono_16_bit_decodes(void)
{
	const int16_t samples[3] = { -32768, 0, 16384 };
	struct wav_file file;
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_sound sound = { 0 };

	start(&file, "WAVE");
	put_fmt(&file, 1, 1, 44100, 16);
	put_data(&file, samples, sizeof(samples));
	finish(&file);
	VOE_TEST_CHECK_INT((int)decode(&file, file.size, arena, &sound),
			   (int)VOE_BASE_OK);
	VOE_TEST_CHECK_INT((int)sound.rate, 44100);
	VOE_TEST_CHECK_INT((int)sound.channels, 1);
	VOE_TEST_CHECK_INT((int)sound.frames, 3);
	VOE_TEST_CHECK_FLOAT(sound.samples[0], -1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sound.samples[1], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(sound.samples[2], 0.5f, 0.0f);
	voe_base_arena_destroy(arena);
}

// Decodes to exactly the floats that went in, two frames of two channels.
static void floats_round_trip(const struct wav_file *file, const float *want)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_sound sound = { 0 };

	VOE_TEST_CHECK_INT((int)decode(file, file->size, arena, &sound),
			   (int)VOE_BASE_OK);
	VOE_TEST_CHECK_INT((int)sound.rate, 48000);
	VOE_TEST_CHECK_INT((int)sound.channels, 2);
	VOE_TEST_CHECK_INT((int)sound.frames, 2);
	VOE_TEST_CHECK(memcmp(sound.samples, want, 4 * sizeof(float)) == 0);
	voe_base_arena_destroy(arena);
}

static const float STEREO[4] = { 0.25f, -0.75f, 1.0f, -0.125f };

static void stereo_float_round_trips(void)
{
	struct wav_file file;

	start(&file, "WAVE");
	put_fmt(&file, 3, 2, 48000, 32);
	put_data(&file, STEREO, sizeof(STEREO));
	finish(&file);
	floats_round_trip(&file, STEREO);
}

static void extensible_float_decodes(void)
{
	static const uint8_t float_guid[16] = { 0x03, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x10, 0x00,
						0x80, 0x00, 0x00, 0xaa,
						0x00, 0x38, 0x9b, 0x71 };
	struct wav_file file;

	start(&file, "WAVE");
	put(&file, "fmt ", 4);
	put32(&file, 40);
	put16(&file, 0xfffe);
	put16(&file, 2);
	put32(&file, 48000);
	put32(&file, 48000 * 8);
	put16(&file, 8);
	put16(&file, 32);
	put16(&file, 22);
	put16(&file, 32);
	put32(&file, 3);
	put(&file, float_guid, sizeof(float_guid));
	put_data(&file, STEREO, sizeof(STEREO));
	finish(&file);
	floats_round_trip(&file, STEREO);
}

// Odd-sized, so its pad byte has to be stepped over too.
static void a_list_chunk_is_skipped(void)
{
	struct wav_file file;

	start(&file, "WAVE");
	put_fmt(&file, 3, 2, 48000, 32);
	put(&file, "LIST", 4);
	put32(&file, 5);
	put(&file, "INFOx\0", 6);
	put_data(&file, STEREO, sizeof(STEREO));
	finish(&file);
	floats_round_trip(&file, STEREO);
}

static void eight_bit_is_unsupported(void)
{
	const uint8_t samples[2] = { 0x80, 0xff };
	struct wav_file file;

	start(&file, "WAVE");
	put_fmt(&file, 1, 1, 8000, 8);
	put_data(&file, samples, sizeof(samples));
	finish(&file);
	refuses(&file, file.size, VOE_BASE_ERROR_UNSUPPORTED);
}

static void three_channels_are_unsupported(void)
{
	const int16_t samples[3] = { 1, 2, 3 };
	struct wav_file file;

	start(&file, "WAVE");
	put_fmt(&file, 1, 3, 44100, 16);
	put_data(&file, samples, sizeof(samples));
	finish(&file);
	refuses(&file, file.size, VOE_BASE_ERROR_UNSUPPORTED);
}

// The RIFF size still claims the whole file; the bytes stop short of it.
static void truncated_data_is_malformed(void)
{
	struct wav_file file;

	start(&file, "WAVE");
	put_fmt(&file, 3, 2, 48000, 32);
	put_data(&file, STEREO, sizeof(STEREO));
	finish(&file);
	refuses(&file, file.size - 3, VOE_BASE_ERROR_MALFORMED);
}

static void an_avi_is_malformed(void)
{
	struct wav_file file;

	start(&file, "AVI ");
	put_fmt(&file, 3, 2, 48000, 32);
	put_data(&file, STEREO, sizeof(STEREO));
	finish(&file);
	refuses(&file, file.size, VOE_BASE_ERROR_MALFORMED);
}

int main(void)
{
	mono_16_bit_decodes();
	stereo_float_round_trips();
	extensible_float_decodes();
	a_list_chunk_is_skipped();
	eight_bit_is_unsupported();
	three_channels_are_unsupported();
	truncated_data_is_malformed();
	an_avi_is_malformed();
	return voe_test_result();
}
