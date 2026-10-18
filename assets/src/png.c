// The PNG reader. See include/assets/image.h for what a caller gets, what is
// refused, and why nothing here turns the picture over.
//
// A PNG IS A SIGNATURE AND THEN A LIST OF CHUNKS, WHICH IS WHY THIS FILE IS FLAT
// AND HAS NO RECURSION IN IT. Every chunk carries its own length, so walking the
// file is a loop that adds a number to an offset — there is no nesting to
// descend into and therefore nothing for ADR-0043's explicit-stack rule to bite
// on. That rule is aimed at glTF and JSON, and card 018 is where it earns its
// keep. It is named here so that the next reader knows it was considered and not
// forgotten.
//
// THE CRC IS SHARED WITH THE WRITER: src/png_write.c stamps the chunks it emits
// with voe_assets_png_crc(), declared in src/png_crc.h and implemented below,
// where the table already was.
//
// THE LENGTH IN A CHUNK HEADER IS THE FIRST HOSTILE NUMBER IN THE FILE, AND
// EVERY ONE AFTER IT IS TOO. The rule this file follows without exception: a
// number read out of the file is compared against what is actually left before
// it is used for anything. Not after, and never as part of the expression that
// uses it — `offset + length > size` overflows and says yes, which is the bug
// that check was written to prevent, so it is spelled `length > size - offset`
// with `offset <= size` already known.
//
// EVERY REFUSAL PRINTS ONE LINE, AND ONLY THAT ONE (ADR-0445 point 2): the
// cause in the artist's words with the numbers, what to do, then the format's
// own words as `(PNG: …)`. The picture is never named here; whoever opened it
// pushed it as the report's subject. inflate.c says nothing, so a stream that
// will not inflate is reported here, as damage.
#include <assets/image.h>

#include <base/assert.h>
#include <base/report.h>

#include "inflate.h"
#include "png_crc.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// The artist's half of every refusal that is damage rather than a choice.
#define DAMAGED "the picture is damaged; save it again as PNG "

// The eight bytes every PNG starts with. The CR-LF-and-Ctrl-Z in the middle is
// deliberate on the format's part: it is there so that a PNG sent through a
// transfer that mangles line endings stops looking like a PNG.
static const uint8_t SIGNATURE[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a,
				      0x0a };

#define COLOUR_GREY 0
#define COLOUR_RGB 2
#define COLOUR_PALETTE 3
#define COLOUR_GREY_ALPHA 4
#define COLOUR_RGBA 6

// CRC-32 as PNG defines it, computed a byte at a time against a table built on
// first use. See src/png_crc.h for why the writer calls this one rather than
// carrying its own.
//
// IT IS CHECKED ON EVERY CHUNK, WHICH IS WHY inflate.c DOES NOT CHECK ITS
// ADLER-32. One integrity check over the compressed bytes is enough, and this is
// the one that also covers the header, the palette and the chunk boundaries.
static uint32_t crc_table[256];
static bool crc_table_built;

static void build_crc_table(void)
{
	for (uint32_t n = 0; n < 256; n++) {
		uint32_t c = n;

		for (int k = 0; k < 8; k++)
			c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
	crc_table_built = true;
}

uint32_t voe_assets_png_crc(const uint8_t *bytes, size_t count)
{
	uint32_t c = 0xffffffffu;

	if (!crc_table_built)
		build_crc_table();

	for (size_t i = 0; i < count; i++)
		c = crc_table[(c ^ bytes[i]) & 0xff] ^ (c >> 8);

	return c ^ 0xffffffffu;
}

static uint32_t be32(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
	       ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

// The one exit of every refusal: its line, then its code. The line is
// composed here because report.h takes no va_list; report.h's cut still holds.
[[gnu::format(printf, 3, 4)]]
static bool fail(voe_base_error *error, voe_base_error code, const char *words,
		 ...)
{
	char line[512];
	va_list arguments;

	VOE_BASE_ASSERT(words != NULL, "a refusal says why");
	va_start(arguments, words);
	(void)vsnprintf(line, sizeof(line), words, arguments);
	va_end(arguments);
	VOE_BASE_ERROR("assets", "%s", line);
	if (error != NULL)
		*error = code;
	return false;
}

// Paeth, from the PNG specification. It picks whichever of the three
// neighbouring bytes the linear predictor a + b - c is closest to.
static uint8_t paeth(uint8_t a, uint8_t b, uint8_t c)
{
	int p = (int)a + (int)b - (int)c;
	int pa = p > (int)a ? p - (int)a : (int)a - p;
	int pb = p > (int)b ? p - (int)b : (int)b - p;
	int pc = p > (int)c ? p - (int)c : (int)c - p;

	if (pa <= pb && pa <= pc)
		return a;
	if (pb <= pc)
		return b;
	return c;
}

// Undo the per-scanline filter, in place, over the whole decompressed block.
//
// EACH ROW IS PREFIXED BY ITS FILTER BYTE, WHICH IS WHY THE DECOMPRESSED SIZE IS
// height * (1 + width * pixel_bytes) AND NOT THE SIZE OF THE PICTURE. The filters
// refer to the byte one pixel to the left and the row above, both already
// unfiltered, so this walks forwards and can never read a byte it has not
// already fixed. At depth 16 a pixel is two bytes a sample and "left" is that
// far back; the filters still work a byte at a time.
[[nodiscard]] static bool unfilter(uint8_t *raw, uint32_t width,
				   uint32_t height, uint32_t pixel_bytes)
{
	size_t stride = (size_t)width * pixel_bytes;
	uint8_t *previous = NULL;
	uint8_t *row = raw;

	for (uint32_t y = 0; y < height; y++) {
		uint8_t filter = row[0];
		uint8_t *current = row + 1;

		for (size_t i = 0; i < stride; i++) {
			uint8_t left = i >= pixel_bytes ? current[i - pixel_bytes]
							: 0;
			uint8_t up = previous != NULL ? previous[i] : 0;
			uint8_t up_left = (previous != NULL && i >= pixel_bytes)
						  ? previous[i - pixel_bytes]
						  : 0;

			switch (filter) {
			case 0:
				break;
			case 1:
				current[i] += left;
				break;
			case 2:
				current[i] += up;
				break;
			case 3:
				current[i] += (uint8_t)(((int)left + (int)up) / 2);
				break;
			case 4:
				current[i] += paeth(left, up, up_left);
				break;
			default:
				// A filter byte the format does not define.
				return false;
			}
		}

		previous = current;
		row = current + stride;
	}

	return true;
}

// A row of 16-bit samples, big-endian as PNG stores them, made 8 bits in place:
// sample i is written at byte i, which is never past the bytes it came from.
// Each is rounded to the nearest 8-bit value, (v + 128) / 257, as 0446 point 2
// says, rather than cut to its high byte.
static void narrow_to_8_bits(uint8_t *row, size_t samples)
{
	for (size_t i = 0; i < samples; i++) {
		uint32_t value = ((uint32_t)row[i * 2] << 8) | row[i * 2 + 1];

		row[i] = (uint8_t)((value + 128) / 257);
	}
}

// How many samples a pixel has in the file, which at depth 8 is its bytes.
// Zero for a colour type this decoder does not read.
static uint32_t channels_of(uint8_t colour_type)
{
	switch (colour_type) {
	case COLOUR_GREY:
		return 1;
	case COLOUR_RGB:
		return 3;
	case COLOUR_PALETTE:
		return 1;
	case COLOUR_GREY_ALPHA:
		return 2;
	case COLOUR_RGBA:
		return 4;
	default:
		return 0;
	}
}

// Widen one unfiltered row into RGBA8. The palette case is the only one that can
// fail, because it is the only one that indexes something.
[[nodiscard]] static bool widen(uint8_t *out, const uint8_t *in, uint32_t width,
				uint8_t colour_type, const uint8_t *palette,
				uint32_t palette_count,
				const uint8_t *palette_alpha,
				uint32_t palette_alpha_count)
{
	for (uint32_t x = 0; x < width; x++) {
		uint8_t *pixel = out + (size_t)x * 4;

		switch (colour_type) {
		case COLOUR_GREY:
			pixel[0] = pixel[1] = pixel[2] = in[x];
			pixel[3] = 0xff;
			break;
		case COLOUR_RGB:
			pixel[0] = in[(size_t)x * 3 + 0];
			pixel[1] = in[(size_t)x * 3 + 1];
			pixel[2] = in[(size_t)x * 3 + 2];
			pixel[3] = 0xff;
			break;
		case COLOUR_PALETTE: {
			uint32_t index = in[x];

			// AN INDEX PAST THE PALETTE IS THE ATTACK THIS COLOUR
			// TYPE HAS. Every byte of the image data is an offset
			// into an array whose length came out of the same file.
			if (index >= palette_count)
				return false;
			pixel[0] = palette[index * 3 + 0];
			pixel[1] = palette[index * 3 + 1];
			pixel[2] = palette[index * 3 + 2];
			// tRNS is optional and may be shorter than the palette;
			// entries it does not reach are opaque.
			pixel[3] = index < palette_alpha_count
					   ? palette_alpha[index]
					   : 0xff;
			break;
		}
		case COLOUR_GREY_ALPHA:
			pixel[0] = pixel[1] = pixel[2] = in[(size_t)x * 2 + 0];
			pixel[3] = in[(size_t)x * 2 + 1];
			break;
		case COLOUR_RGBA:
			memcpy(pixel, in + (size_t)x * 4, 4);
			break;
		default:
			return false;
		}
	}

	return true;
}

bool voe_assets_png_decode(const uint8_t *bytes, size_t size,
			   voe_base_arena *arena, voe_assets_image *image,
			   voe_base_error *error)
{
	size_t offset;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channels = 0;
	uint32_t sample_bytes = 1;
	size_t row_bytes;
	uint8_t colour_type = 0;
	bool seen_header = false;
	bool seen_end = false;
	const uint8_t *palette = NULL;
	uint32_t palette_count = 0;
	const uint8_t *palette_alpha = NULL;
	uint32_t palette_alpha_count = 0;
	uint8_t *compressed;
	size_t compressed_size = 0;
	size_t compressed_capacity = 0;
	uint8_t *raw;
	size_t raw_size;
	uint8_t *pixels;

	VOE_BASE_DEBUG_ASSERT(bytes != NULL, "decoding a PNG from nothing");
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "decoding a PNG with no arena");
	VOE_BASE_DEBUG_ASSERT(image != NULL, "decoding a PNG into nothing");

	if (size < sizeof(SIGNATURE) ||
	    memcmp(bytes, SIGNATURE, sizeof(SIGNATURE)) != 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: no PNG signature)");

	// FIRST PASS: find the header, the palette, and how much compressed data
	// there is. Nothing is decompressed until the total is known, because
	// IDAT may be split across any number of chunks and they have to be fed
	// to the decompressor as one stream.
	offset = sizeof(SIGNATURE);
	while (offset + 12 <= size) {
		uint32_t length = be32(bytes + offset);
		const uint8_t *type = bytes + offset + 4;
		const uint8_t *data = bytes + offset + 8;

		// 12 is the chunk's own overhead: length, type and CRC. The
		// subtraction cannot wrap because the loop condition just
		// established that offset + 12 fits.
		if (length > size - offset - 12)
			return fail(error, VOE_BASE_ERROR_MALFORMED,
				    DAMAGED "(PNG: a chunk runs past the end "
					    "of the file)");

		if (voe_assets_png_crc(type, (size_t)length + 4) !=
		    be32(data + length))
			return fail(error, VOE_BASE_ERROR_MALFORMED,
				    DAMAGED "(PNG: a chunk's CRC does not "
					    "match)");

		if (memcmp(type, "IHDR", 4) == 0) {
			if (seen_header || length != 13)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: IHDR is repeated or "
						    "not 13 bytes)");
			width = be32(data);
			height = be32(data + 4);
			colour_type = data[9];

			if (width == 0 || height == 0)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: IHDR gives a width "
						    "or height of 0)");
			// A limit the engine keeps (ADR-0443), so a choice
			// and not damage: UNSUPPORTED, in the limit's words.
			if (width > VOE_ASSETS_IMAGE_MAX_SIDE ||
			    height > VOE_ASSETS_IMAGE_MAX_SIDE)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED,
					    "the picture is %" PRIu32
					    " × %" PRIu32
					    " pixels, the most the engine "
					    "takes is %d a side; scale it down "
					    "(PNG: IHDR width and height)",
					    width, height,
					    VOE_ASSETS_IMAGE_MAX_SIDE);
			// Compression 0 and filter 0 are the only ones the
			// format defines; anything else is a corrupt header
			// rather than a variant.
			if (data[10] != 0 || data[11] != 0)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: compression %u, "
						    "filter method %u)",
					    (unsigned)data[10],
					    (unsigned)data[11]);
			// Depth and interlace are well-formed values this
			// decoder declines, so they are UNSUPPORTED and not
			// MALFORMED. image.h says which. A palette at 16
			// is one the format forbids, refused the same way.
			if (data[8] != 8 && data[8] != 16)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED,
					    "the picture is %u-bit, the engine "
					    "reads 8-bit and 16-bit pictures; "
					    "save it as 8-bit (PNG: bit depth "
					    "%u)",
					    (unsigned)data[8],
					    (unsigned)data[8]);
			if (data[8] == 16 && colour_type == COLOUR_PALETTE)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED,
					    "the picture is a 16-bit palette "
					    "picture, which the engine does not "
					    "read; save it as 8-bit (PNG: bit "
					    "depth 16 with colour type 3)");
			if (data[12] != 0)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED,
					    "the picture is interlaced, which "
					    "the engine does not read; save it "
					    "without interlacing (PNG: "
					    "interlace method %u)",
					    (unsigned)data[12]);

			channels = channels_of(colour_type);
			if (channels == 0)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: colour type %u)",
					    (unsigned)colour_type);
			sample_bytes = data[8] / 8;
			seen_header = true;
		} else if (memcmp(type, "PLTE", 4) == 0) {
			if (!seen_header || length % 3 != 0 || length == 0 ||
			    length > 256 * 3)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: PLTE before IHDR or "
						    "of %" PRIu32 " bytes)",
					    length);
			palette = data;
			palette_count = length / 3;
		} else if (memcmp(type, "tRNS", 4) == 0) {
			if (!seen_header)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: tRNS before IHDR)");
			// Only the palette form is read. The two others give a
			// single colour full transparency, which nothing asks
			// for yet (rule 10).
			if (colour_type == COLOUR_PALETTE) {
				if (length > 256)
					return fail(error,
						    VOE_BASE_ERROR_MALFORMED,
						    DAMAGED "(PNG: tRNS of %" PRIu32
							    " bytes)",
						    length);
				palette_alpha = data;
				palette_alpha_count = length;
			}
		} else if (memcmp(type, "IDAT", 4) == 0) {
			if (!seen_header)
				return fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(PNG: IDAT before IHDR)");
			compressed_capacity += length;
		} else if (memcmp(type, "IEND", 4) == 0) {
			seen_end = true;
			break;
		}

		offset += 12 + (size_t)length;
	}

	// IEND IS REQUIRED, AND REQUIRING IT IS WHAT CATCHES A TRUNCATED FILE
	// WHOSE PICTURE HAPPENED TO ARRIVE WHOLE. Cutting the last twelve bytes
	// off a PNG leaves every chunk this decoder reads intact, so without
	// this the file decodes and nobody is told it was damaged — and a file
	// that lost its tail may equally have lost an IDAT, which would decode
	// to a picture that is quietly missing its bottom half. The end marker
	// is the only thing in the format that says "that was all of it".
	if (!seen_end)
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: no IEND, the file is cut short)");
	if (!seen_header || compressed_capacity == 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: no IHDR or no IDAT)");
	if (colour_type == COLOUR_PALETTE && palette == NULL)
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: colour type 3 with no PLTE)");

	// SECOND PASS: gather the IDAT bytes into one buffer. The capacity was
	// summed above from lengths that were each checked against the file, so
	// the copy below cannot outrun it.
	compressed = voe_base_arena_push(arena, compressed_capacity);
	offset = sizeof(SIGNATURE);
	while (offset + 12 <= size) {
		uint32_t length = be32(bytes + offset);
		const uint8_t *type = bytes + offset + 4;

		if (memcmp(type, "IDAT", 4) == 0) {
			memcpy(compressed + compressed_size, bytes + offset + 8,
			       length);
			compressed_size += length;
		} else if (memcmp(type, "IEND", 4) == 0) {
			break;
		}

		offset += 12 + (size_t)length;
	}

	// One filter byte per row, then the row. Every factor is bounded by
	// the header checks — VOE_ASSETS_IMAGE_MAX_SIDE, channels of at most
	// 4 and two bytes a sample at most — so the largest this can be is
	// about two gigabytes and it cannot overflow.
	row_bytes = 1 + (size_t)width * channels * sample_bytes;
	raw_size = (size_t)height * row_bytes;
	raw = voe_base_arena_push(arena, raw_size);

	if (!voe_assets_inflate(compressed, compressed_size, raw, raw_size,
				NULL))
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: the IDAT data does not inflate to "
				    "the %zu bytes IHDR implies)",
			    raw_size);

	if (!unfilter(raw, width, height, channels * sample_bytes))
		return fail(error, VOE_BASE_ERROR_MALFORMED,
			    DAMAGED "(PNG: a scanline's filter byte is not "
				    "0 to 4)");

	pixels = voe_base_arena_push(arena, (size_t)width * height * 4);
	for (uint32_t y = 0; y < height; y++) {
		uint8_t *in = raw + (size_t)y * row_bytes + 1;
		uint8_t *out = pixels + (size_t)y * width * 4;

		// Only after every row is unfiltered: the filters read the
		// row above at its full 16 bits.
		if (sample_bytes == 2)
			narrow_to_8_bits(in, (size_t)width * channels);
		if (!widen(out, in, width, colour_type, palette, palette_count,
			   palette_alpha, palette_alpha_count))
			return fail(error, VOE_BASE_ERROR_MALFORMED,
				    DAMAGED "(PNG: a palette index past the "
					    "%" PRIu32 " entries of PLTE)",
				    palette_count);
	}

	image->width = width;
	image->height = height;
	image->pixels = pixels;
	return true;
}
