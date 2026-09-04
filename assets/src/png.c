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
// THE LENGTH IN A CHUNK HEADER IS THE FIRST HOSTILE NUMBER IN THE FILE, AND
// EVERY ONE AFTER IT IS TOO. The rule this file follows without exception: a
// number read out of the file is compared against what is actually left before
// it is used for anything. Not after, and never as part of the expression that
// uses it — `offset + length > size` overflows and says yes, which is the bug
// that check was written to prevent, so it is spelled `length > size - offset`
// with `offset <= size` already known.
#include <assets/image.h>

#include <base/assert.h>

#include "inflate.h"

#include <string.h>

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
// first use.
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

static uint32_t crc32_of(const uint8_t *bytes, size_t size)
{
	uint32_t c = 0xffffffffu;

	if (!crc_table_built)
		build_crc_table();

	for (size_t i = 0; i < size; i++)
		c = crc_table[(c ^ bytes[i]) & 0xff] ^ (c >> 8);

	return c ^ 0xffffffffu;
}

static uint32_t be32(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
	       ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static bool fail(voe_base_error *error, voe_base_error code)
{
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
// height * (1 + width * channels) AND NOT THE SIZE OF THE PICTURE. The filters
// refer to the byte to the left and the row above, both already unfiltered, so
// this walks forwards and can never read a byte it has not already fixed.
[[nodiscard]] static bool unfilter(uint8_t *raw, uint32_t width,
				   uint32_t height, uint32_t channels)
{
	size_t stride = (size_t)width * channels;
	uint8_t *previous = NULL;
	uint8_t *row = raw;

	for (uint32_t y = 0; y < height; y++) {
		uint8_t filter = row[0];
		uint8_t *current = row + 1;

		for (size_t i = 0; i < stride; i++) {
			uint8_t left = i >= channels ? current[i - channels] : 0;
			uint8_t up = previous != NULL ? previous[i] : 0;
			uint8_t up_left = (previous != NULL && i >= channels)
						  ? previous[i - channels]
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

// How many bytes a pixel takes in the file, for the colour types read at depth
// 8. Zero for one this decoder does not read.
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
		return fail(error, VOE_BASE_ERROR_MALFORMED);

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
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		if (crc32_of(type, (size_t)length + 4) !=
		    be32(data + length))
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		if (memcmp(type, "IHDR", 4) == 0) {
			if (seen_header || length != 13)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			width = be32(data);
			height = be32(data + 4);
			colour_type = data[9];

			if (width == 0 || height == 0 ||
			    width > VOE_ASSETS_IMAGE_MAX_SIDE ||
			    height > VOE_ASSETS_IMAGE_MAX_SIDE)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			// Compression 0 and filter 0 are the only ones the
			// format defines; anything else is a corrupt header
			// rather than a variant.
			if (data[10] != 0 || data[11] != 0)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			// Depth and interlace are well-formed values this
			// decoder declines, so they are UNSUPPORTED and not
			// MALFORMED. image.h says which.
			if (data[8] != 8)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED);
			if (data[12] != 0)
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

			channels = channels_of(colour_type);
			if (channels == 0)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			seen_header = true;
		} else if (memcmp(type, "PLTE", 4) == 0) {
			if (!seen_header || length % 3 != 0 || length == 0 ||
			    length > 256 * 3)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			palette = data;
			palette_count = length / 3;
		} else if (memcmp(type, "tRNS", 4) == 0) {
			if (!seen_header)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			// Only the palette form is read. The two others give a
			// single colour full transparency, which nothing asks
			// for yet (rule 10).
			if (colour_type == COLOUR_PALETTE) {
				if (length > 256)
					return fail(error,
						    VOE_BASE_ERROR_MALFORMED);
				palette_alpha = data;
				palette_alpha_count = length;
			}
		} else if (memcmp(type, "IDAT", 4) == 0) {
			if (!seen_header)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
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
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	if (!seen_header || compressed_capacity == 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	if (colour_type == COLOUR_PALETTE && palette == NULL)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

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

	// One filter byte per row, then the row. Both multiplications are
	// bounded by VOE_ASSETS_IMAGE_MAX_SIDE and channels of at most 4, so
	// the largest this can be is about a gigabyte and it cannot overflow.
	raw_size = (size_t)height * (1 + (size_t)width * channels);
	raw = voe_base_arena_push(arena, raw_size);

	if (!voe_assets_inflate(compressed, compressed_size, raw, raw_size,
				error))
		return false;

	if (!unfilter(raw, width, height, channels))
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	pixels = voe_base_arena_push(arena, (size_t)width * height * 4);
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *in = raw + (size_t)y *
					    (1 + (size_t)width * channels) + 1;
		uint8_t *out = pixels + (size_t)y * width * 4;

		if (!widen(out, in, width, colour_type, palette, palette_count,
			   palette_alpha, palette_alpha_count))
			return fail(error, VOE_BASE_ERROR_MALFORMED);
	}

	image->width = width;
	image->height = height;
	image->pixels = pixels;
	return true;
}
