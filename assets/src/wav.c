// The WAV reader behind assets/sound.h: one walk over the RIFF chunks that
// remembers `fmt ` and `data`, a check of the format, and one of two sample
// conversions into the caller's arena.
//
// Every length read out of the file is checked against what is left of the
// bytes before it is used, and the walk advances at least eight bytes a chunk,
// so it ends within size / 8 steps whatever the file says. Nothing is written
// to `sound` until every check has passed.
//
// Every refusal prints one line (ADR-0445 point 2): the cause in the artist's
// words with the numbers, what to do, then `(WAV: …)` for a developer. The
// sound is never named here; whoever opened it pushed it as the subject.
#include <assets/sound.h>

#include <base/assert.h>
#include <base/report.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define WAV_TAG_PCM 0x0001u
#define WAV_TAG_FLOAT 0x0003u
#define WAV_TAG_EXTENSIBLE 0xfffeu

// The fixed fields of `fmt `: tag, channels, rate, byte rate, block align, bits.
#define WAV_FMT_BYTES 16u
// Extensible adds cbSize, valid bits, channel mask and a 16-byte subformat GUID.
#define WAV_FMT_EXTENSIBLE_BYTES 40u
#define WAV_SUBFORMAT_OFFSET 24u

// The artist's half of every refusal that is damage rather than a choice.
#define DAMAGED "the sound is damaged; export it again "
// What to do about a sample format this reader does not take.
#define EXPORT_AS                                                    \
	"the engine reads 16-bit PCM and 32-bit float; export it as " \
	"16-bit PCM or 32-bit float WAV"

// The fourteen bytes every KSDATAFORMAT_SUBTYPE GUID shares after its tag.
static const uint8_t SUBFORMAT_TAIL[14] = { 0x00, 0x00, 0x00, 0x00, 0x10,
					    0x00, 0x80, 0x00, 0x00, 0xaa,
					    0x00, 0x38, 0x9b, 0x71 };

struct wav_chunks {
	const uint8_t *fmt;
	uint32_t fmt_size;
	const uint8_t *data;
	uint32_t data_size;
};

static uint16_t le16(const uint8_t *bytes)
{
	return (uint16_t)(bytes[0] | bytes[1] << 8);
}

static uint32_t le32(const uint8_t *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
	       (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

// The one exit of every refusal: its line, then its code. The line is
// composed here because report.h takes no va_list; report.h's cut still holds.
[[gnu::format(printf, 3, 4)]]
static bool refuse(voe_base_error *error, voe_base_error code,
		   const char *words, ...)
{
	char line[512];
	va_list arguments;

	VOE_BASE_ASSERT(words != NULL, "a refusal names its reason");
	va_start(arguments, words);
	(void)vsnprintf(line, sizeof(line), words, arguments);
	va_end(arguments);
	VOE_BASE_ERROR("assets", "%s", line);
	if (error != NULL)
		*error = code;
	return false;
}

// A format tag as an artist's export dialog names it, NULL for one it does
// not. Only the tags a tool is likely to offer are named.
static const char *tag_name(uint16_t tag)
{
	switch (tag) {
	case WAV_TAG_PCM:
		return "PCM";
	case WAV_TAG_FLOAT:
		return "float";
	case 0x0002u:
		return "ADPCM";
	case 0x0006u:
		return "A-law";
	case 0x0007u:
		return "µ-law";
	case 0x0011u:
		return "IMA ADPCM";
	case 0x0055u:
		return "MP3";
	default:
		return NULL;
	}
}

// The sample format refused by name: depth and kind for PCM and float, the
// codec's name for the rest, and the tag for a developer either way.
static bool refuse_sample_format(voe_base_error *error, uint16_t tag,
				 uint16_t bits)
{
	const char *name = tag_name(tag);

	if (tag == WAV_TAG_PCM || tag == WAV_TAG_FLOAT)
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "the sound is %u-bit %s; " EXPORT_AS
			      " (WAV: format tag %u, %u bits)",
			      (unsigned)bits, name, (unsigned)tag,
			      (unsigned)bits);
	if (name != NULL)
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "the sound is %s; " EXPORT_AS
			      " (WAV: format tag %u)",
			      name, (unsigned)tag);
	return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
		      "the sound is in a format the engine does not read; "
		      EXPORT_AS " (WAV: format tag or subformat %u)",
		      (unsigned)tag);
}

// Walks the chunks between byte 12 and `end`, keeping the first `fmt ` and the
// first `data`. A chunk whose body runs past `end` is malformed; its pad byte
// may be missing only when nothing follows it.
static bool find_chunks(const uint8_t *bytes, size_t end,
			struct wav_chunks *chunks, voe_base_error *error)
{
	size_t pos = 12;

	VOE_BASE_ASSERT(bytes != NULL && chunks != NULL, "find_chunks inputs");
	while (pos + 8 <= end) {
		const uint32_t chunk_size = le32(bytes + pos + 4);
		const uint8_t *body = bytes + pos + 8;

		if (chunk_size > end - pos - 8)
			return refuse(error, VOE_BASE_ERROR_MALFORMED,
				      DAMAGED "(WAV: a chunk runs past the end "
					      "of the file)");
		if (memcmp(bytes + pos, "fmt ", 4) == 0 && chunks->fmt == NULL) {
			chunks->fmt = body;
			chunks->fmt_size = chunk_size;
		} else if (memcmp(bytes + pos, "data", 4) == 0 &&
			   chunks->data == NULL) {
			chunks->data = body;
			chunks->data_size = chunk_size;
		}
		pos += 8 + (size_t)chunk_size + (chunk_size & 1u);
	}
	VOE_BASE_ASSERT(pos >= 12, "the walk only moves forward");
	if (chunks->fmt == NULL || chunks->data == NULL)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      DAMAGED "(WAV: no fmt or no data chunk)");
	return true;
}

// The format tag the samples are actually in: the plain tag, or the one inside
// an extensible header's subformat. Zero when the subformat is not one of ours.
static uint16_t sample_tag(const struct wav_chunks *chunks)
{
	const uint16_t tag = le16(chunks->fmt);
	const uint8_t *subformat = chunks->fmt + WAV_SUBFORMAT_OFFSET;

	VOE_BASE_ASSERT(chunks->fmt_size >= WAV_FMT_BYTES, "fmt was measured");
	if (tag != WAV_TAG_EXTENSIBLE)
		return tag;
	VOE_BASE_ASSERT(chunks->fmt_size >= WAV_FMT_EXTENSIBLE_BYTES,
			"extensible fmt was measured");
	if (memcmp(subformat + 2, SUBFORMAT_TAIL, sizeof(SUBFORMAT_TAIL)) != 0)
		return 0;
	return le16(subformat);
}

// Checks `fmt ` and fills everything of `out` but the samples.
static bool read_format(const struct wav_chunks *chunks, voe_assets_sound *out,
			uint16_t *tag, voe_base_error *error)
{
	uint16_t bits;

	VOE_BASE_ASSERT(chunks->fmt != NULL && out != NULL, "read_format inputs");
	if (chunks->fmt_size < WAV_FMT_BYTES ||
	    (le16(chunks->fmt) == WAV_TAG_EXTENSIBLE &&
	     chunks->fmt_size < WAV_FMT_EXTENSIBLE_BYTES))
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      DAMAGED "(WAV: fmt chunk of %" PRIu32
				      " bytes, too short for its tag)",
			      chunks->fmt_size);
	out->channels = le16(chunks->fmt + 2);
	out->rate = le32(chunks->fmt + 4);
	bits = le16(chunks->fmt + 14);
	if (out->channels == 0 || out->rate == 0)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      DAMAGED "(WAV: %" PRIu32 " channels at %" PRIu32
				      " Hz)",
			      out->channels, out->rate);
	*tag = sample_tag(chunks);
	if (!(*tag == WAV_TAG_PCM && bits == 16) &&
	    !(*tag == WAV_TAG_FLOAT && bits == 32))
		return refuse_sample_format(error, *tag, bits);
	if (out->channels > 2)
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "the sound has %" PRIu32 " channels, the most "
			      "the engine takes is 2; mix it to stereo "
			      "(WAV: %" PRIu32 " channels)",
			      out->channels, out->channels);
	VOE_BASE_ASSERT(out->channels <= 2 && out->rate > 0, "format checked");
	return true;
}

static void convert_pcm16(const uint8_t *data, size_t count, float *samples)
{
	VOE_BASE_ASSERT(data != NULL && samples != NULL, "pcm16 buffers");
	for (size_t i = 0; i < count; i++)
		samples[i] = (float)(int16_t)le16(data + i * 2) / 32768.0f;
	VOE_BASE_ASSERT(count == 0 || samples[0] >= -1.0f, "in range");
}

static void convert_float32(const uint8_t *data, size_t count, float *samples)
{
	VOE_BASE_ASSERT(data != NULL && samples != NULL, "float32 buffers");
	for (size_t i = 0; i < count; i++) {
		const uint32_t word = le32(data + i * 4);

		memcpy(&samples[i], &word, sizeof(word));
	}
	VOE_BASE_ASSERT(sizeof(float) == sizeof(uint32_t), "float is 32 bits");
}

bool voe_assets_wav_decode(const uint8_t *bytes, size_t size,
			   voe_base_arena *arena, voe_assets_sound *sound,
			   voe_base_error *error)
{
	struct wav_chunks chunks = { 0 };
	voe_assets_sound out = { 0 };
	uint16_t tag = 0;
	size_t end;
	size_t sample_bytes;
	size_t count;

	VOE_BASE_ASSERT(bytes != NULL || size == 0, "bytes for a size");
	VOE_BASE_ASSERT(arena != NULL && sound != NULL, "an arena and a sound");
	if (size < 12 || memcmp(bytes, "RIFF", 4) != 0 ||
	    memcmp(bytes + 8, "WAVE", 4) != 0)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      DAMAGED "(WAV: not a RIFF/WAVE file)");
	end = size - 8 < le32(bytes + 4) ? size : (size_t)le32(bytes + 4) + 8;
	if (!find_chunks(bytes, end, &chunks, error) ||
	    !read_format(&chunks, &out, &tag, error))
		return false;
	sample_bytes = tag == WAV_TAG_PCM ? 2 : 4;
	if (chunks.data_size % (sample_bytes * out.channels) != 0)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      DAMAGED "(WAV: data of %" PRIu32
				      " bytes is not a whole number of "
				      "%zu-byte frames)",
			      chunks.data_size, sample_bytes * out.channels);
	count = chunks.data_size / sample_bytes;
	out.frames = count / out.channels;
	out.samples = voe_base_arena_push(arena, (count ? count : 1) *
							 sizeof(float));
	if (tag == WAV_TAG_PCM)
		convert_pcm16(chunks.data, count, out.samples);
	else
		convert_float32(chunks.data, count, out.samples);
	*sound = out;
	return true;
}
