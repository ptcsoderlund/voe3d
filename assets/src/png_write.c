// The PNG writer. See include/assets/image.h for what a caller hands over, what
// comes back, and why colour is written exactly as it arrives.
//
// A PNG IS A SIGNATURE AND THEN A LIST OF CHUNKS, AND THIS ONE WRITES FOUR
// THINGS: the signature, IHDR, one IDAT and IEND. The CRC over each chunk is
// src/png_crc.h's — the same one the reader checks with, and the reason it was
// lifted out of src/png.c.
//
// ONE IDAT AND NOT MANY, BECAUSE THE FORMAT ONLY NEEDS MANY WHEN THE WRITER IS
// STREAMING. Splitting the compressed stream across chunks exists so an encoder
// can emit bytes before it knows how many there will be; this one has the whole
// zlib stream in memory before it writes a single chunk, so a split would buy
// nothing and cost a loop. Readers must accept both — ours does, and so the
// decoder beside this is not the one proving it.
//
// FILTER 0 ON EVERY ROW, WHICH IS "NO FILTER AT ALL". The filters exist to make
// the DEFLATE that follows them cheaper, and what this engine writes is a
// screenshot: mostly flat colour, where the match finder in src/deflate.c
// already finds the runs and a predictor would mostly turn them into a different
// set of runs of the same length. Choosing a filter per row is a heuristic and a
// second thing to get wrong for a file nobody transmits. The decoder next door
// reads all five, so the day a measurement asks for them, nothing here has to be
// read back — only the byte at the start of each row changes.
//
// EVERY BUFFER PUSHED HERE IS SIZED BEFORE IT IS WRITTEN, AND THE SIZE COMES
// FROM ONE OF THREE PLACES. The filtered rows are height * (1 + width * 4),
// which is the format's own definition of what gets compressed. The zlib stream
// is src/deflate.h's worst case and is pushed by src/deflate.c itself. The file
// is those bytes plus a fixed overhead — eight for the signature and twelve for
// each chunk's length, type and CRC, plus IHDR's thirteen — so the only number
// that is not a constant is one this file was handed. `fits()` is where the
// multiplications that produce them are checked, once, before anything is
// pushed; nothing below it re-derives a size.
#include <assets/image.h>

#include <base/assert.h>

#include "deflate.h"
#include "png_crc.h"

#include <string.h>

// The eight bytes every PNG starts with. Spelled out here as well as in
// src/png.c: a reader looks for the magic number in the file that writes it, and
// sharing eight constant bytes across two files buys less than that costs.
static const uint8_t SIGNATURE[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a,
				      0x0a };

// A chunk's own bytes beside its data: four of length, four of type, four of
// CRC.
#define CHUNK_OVERHEAD 12

// IHDR's data is always these thirteen bytes for this writer.
#define IHDR_SIZE 13

// Colour type 6 is RGBA; the eight is bits per channel. image.h says why there
// is no other choice here.
#define COLOUR_RGBA 6
#define BIT_DEPTH 8

static void put_be32(uint8_t *at, uint32_t value)
{
	at[0] = (uint8_t)(value >> 24);
	at[1] = (uint8_t)(value >> 16);
	at[2] = (uint8_t)(value >> 8);
	at[3] = (uint8_t)value;
}

// Write one chunk at `at` and answer how many bytes it took. `type` is four
// characters; `data` may be NULL only when `count` is zero, which is IEND.
//
// THE CRC COVERS THE TYPE AND THE DATA AND NOT THE LENGTH, so it is taken over
// the bytes just written starting four in — one call over one contiguous run,
// which is also why the type is copied before the CRC is computed rather than
// hashed from the caller's string.
static size_t put_chunk(uint8_t *at, const char *type, const uint8_t *data,
			size_t count)
{
	put_be32(at, (uint32_t)count);
	memcpy(at + 4, type, 4);
	if (count != 0)
		memcpy(at + 8, data, count);
	put_be32(at + 8 + count, voe_assets_png_crc(at + 4, 4 + count));

	return CHUNK_OVERHEAD + count;
}

// Work out the filtered-row stride and total, and say no if either multiply
// would not fit.
//
// IT IS THE ONLY PLACE A SIZE IS DERIVED, AND IT IS REACHABLE: width and height
// are four bytes each, so width * 4 * height can be 2^66 on a picture nobody
// could hold and still not be a wrong picture. The last bound is the one that
// matters least and is easiest to get wrong — the file buffer is the rows plus
// deflate's worst case, about nine eighths, plus a constant, so refusing
// anything above half of SIZE_MAX leaves every later addition room without a
// second checked multiply.
[[nodiscard]] static bool fits(uint32_t width, uint32_t height, size_t *stride,
			       size_t *total)
{
	// The height is never zero here: the only caller asserts it, and that
	// assert survives NDEBUG precisely so this division does not become one
	// by zero in a release build.
	VOE_BASE_ASSERT(height != 0, "measuring a PNG with no height");

	if ((size_t)width > (SIZE_MAX - 1) / 4)
		return false;
	*stride = 1 + (size_t)width * 4;

	if (*stride > SIZE_MAX / height)
		return false;
	*total = *stride * height;

	return *total <= SIZE_MAX / 2 - 128;
}

bool voe_assets_png_encode(voe_base_arena *arena, voe_assets_image image,
			   voe_assets_bytes *out, voe_base_error *error)
{
	uint8_t header[IHDR_SIZE];
	voe_assets_deflate_result compressed;
	size_t stride;
	size_t total;
	size_t capacity;
	size_t at;
	uint8_t *rows;
	uint8_t *bytes;

	// PLAIN ASSERTS, NOT DEBUG ONES, AND THE HEIGHT IS WHY. fits() below
	// divides by the height; compiled out under NDEBUG, a zero height is a
	// division by zero in the build a user runs rather than an abort that
	// names the caller's bug. The other three cost a comparison each and
	// base/assert.h says which name to reach for when they do.
	VOE_BASE_ASSERT(arena != NULL, "encoding a PNG with no arena");
	VOE_BASE_ASSERT(out != NULL, "encoding a PNG into nothing");
	VOE_BASE_ASSERT(image.pixels != NULL, "encoding a PNG of nothing");
	VOE_BASE_ASSERT(image.width != 0 && image.height != 0,
			"encoding a PNG with no width or no height");

	if (!fits(image.width, image.height, &stride, &total)) {
		if (error != NULL)
			*error = VOE_BASE_ERROR_REFUSED;
		return false;
	}

	// THE FILTER BYTE IS WHY THE ROWS ARE COPIED RATHER THAN COMPRESSED
	// WHERE THEY LIE: every row in the compressed stream is one byte of
	// filter type followed by the pixels, so the buffer is one byte a row
	// longer than the picture and the pixels cannot simply be handed on.
	rows = voe_base_arena_push(arena, total);
	for (uint32_t y = 0; y < image.height; y++) {
		uint8_t *row = rows + (size_t)y * stride;

		row[0] = 0;
		memcpy(row + 1, image.pixels + (size_t)y * (stride - 1),
		       stride - 1);
	}

	voe_assets_deflate(arena, rows, total, &compressed);

	put_be32(header, image.width);
	put_be32(header + 4, image.height);
	header[8] = BIT_DEPTH;
	header[9] = COLOUR_RGBA;
	header[10] = 0; // compression: DEFLATE, the only one defined
	header[11] = 0; // filter method: the five per-row filters, the only one
	header[12] = 0; // interlace: none

	capacity = sizeof(SIGNATURE) + CHUNK_OVERHEAD + IHDR_SIZE +
		   CHUNK_OVERHEAD + compressed.count + CHUNK_OVERHEAD;
	bytes = voe_base_arena_push(arena, capacity);

	memcpy(bytes, SIGNATURE, sizeof(SIGNATURE));
	at = sizeof(SIGNATURE);
	at += put_chunk(bytes + at, "IHDR", header, sizeof(header));
	at += put_chunk(bytes + at, "IDAT", compressed.bytes, compressed.count);
	at += put_chunk(bytes + at, "IEND", NULL, 0);

	VOE_BASE_ASSERT(at == capacity, "the PNG writer miscounted");

	out->bytes = bytes;
	out->count = at;

	return true;
}
