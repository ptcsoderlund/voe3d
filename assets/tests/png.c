// THE PNG READER, AGAINST FILES A REAL ENCODER PRODUCED AND FILES NOBODY WOULD
// PRODUCE ON PURPOSE.
//
// The good ones below were made by zlib and are byte-for-byte what any encoder
// emits, so this exercises the Huffman path rather than a hand-rolled stream a
// test author knew how to write. RGB_PNG uses a different filter on each of its
// five rows — none, Sub, Up, Average, Paeth — because unfilter() is five
// separate arithmetic mistakes waiting to happen and a picture that uses one
// filter proves one of them.
//
// THE WRITER IS TESTED BY THE READER ABOVE IT, AND THAT IS THE WHOLE ORACLE
// AVAILABLE HERE. An encoder checked against its own decoder can only prove the
// two agree, so the checks below also read the signature and the IHDR fields out
// of the file by hand — those are the bytes every other program in the world
// looks at first — and tests/deflate.c is where python3's zlib was made to
// accept what the compressor emits. Between them, a file this writer produces
// opens elsewhere.
//
// THE BAD ONES ARE THE POINT OF THE FILE. Card 017 named three — a truncated
// file, a wrong magic number and an absurd declared size — and each is a
// recoverable failure with a test here. A decoder that crashed or allocated four
// gigabytes on any of them would still pass a test that only ever decoded valid
// pictures, which is why the valid ones are the smaller half of this file.
//
// THE 16-BIT AND 4-BIT FILES ARE BUILT HERE, one stored deflate block each with
// the CRC through ../src/png_crc.h, so their samples are exactly the ones
// 0446's rounding is checked on.
//
// EVERY REFUSAL IS ALSO READ BACK AS THE LINE IT KEPT (ADR-0445): `(PNG: ` on
// all of them, and the artist's words on interlace, depth 4 and the huge size.
#include <assets/image.h>
#include <base/report.h>

#include <testing/test.h>

#include <string.h>

#include "../src/png_crc.h"
#include "png_data.inc"

// Big enough for every picture below several times over, and rewound between
// tests rather than grown.
#define SCRATCH (1u << 20)

// A decode that must succeed, checked for the dimensions it should have. The
// arena is handed back rather than destroyed, because image->pixels lives in it
// and the caller has not looked at them yet; every caller destroys it.
[[nodiscard]] static voe_base_arena *decodes_to(const uint8_t *bytes,
						size_t size, uint32_t width,
						uint32_t height,
						voe_assets_image *image)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_png_decode(bytes, size, arena, image, &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_OK);
	VOE_TEST_CHECK_INT((int)image->width, (int)width);
	VOE_TEST_CHECK_INT((int)image->height, (int)height);

	return arena;
}

// Whether the line kept since the last clear contains `words`.
static bool said(const char *words)
{
	const char *kept = voe_base_report_error_first();

	return kept != NULL && strstr(kept, words) != NULL;
}

// A decode that must fail, with the code it must fail with, and it must not
// crash or run away while doing it. The kept line is left for the caller.
static void refuses(const char *what, const uint8_t *bytes, size_t size,
		    voe_base_error expected)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image image = { 0 };
	voe_base_error error = VOE_BASE_OK;

	(void)what;
	voe_base_report_error_clear();
	VOE_TEST_CHECK(!voe_assets_png_decode(bytes, size, arena, &image, &error));
	VOE_TEST_CHECK_INT((int)error, (int)expected);
	// Every refusal says why, with the format's words for a developer.
	VOE_TEST_CHECK(voe_base_report_error_first() != NULL);
	VOE_TEST_CHECK(said("(PNG: "));

	voe_base_arena_destroy(arena);
}

// Five rows, five filters, and every byte compared. This is the test that says
// the filters are right; everything else says the container is.
static void truecolour_with_every_filter(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(RGB_PNG, sizeof(RGB_PNG), 4, 5, &image);

	for (uint32_t y = 0; y < 5; y++) {
		for (uint32_t x = 0; x < 4; x++) {
			const uint8_t *got = image.pixels + (y * 4 + x) * 4;
			const uint8_t *want = RGB_EXPECTED + (y * 4 + x) * 3;

			VOE_TEST_CHECK_INT(got[0], want[0]);
			VOE_TEST_CHECK_INT(got[1], want[1]);
			VOE_TEST_CHECK_INT(got[2], want[2]);
			// No alpha in the file, so an opaque one is invented.
			VOE_TEST_CHECK_INT(got[3], 0xff);
		}
	}

	voe_base_arena_destroy(arena);
}

// The same picture with the compressor turned off, which is DEFLATE's stored
// block. It is a different path through inflate.c and encoders really do emit it
// for data that will not compress.
static void a_stored_deflate_block_decodes(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(RGB_PNG_STORED, sizeof(RGB_PNG_STORED), 4, 5, &image);
	for (uint32_t i = 0; i < 4 * 5; i++) {
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 0], RGB_EXPECTED[i * 3 + 0]);
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 2], RGB_EXPECTED[i * 3 + 2]);
	}

	voe_base_arena_destroy(arena);
}

// Grey widens into three equal channels and an opaque alpha.
static void greyscale_widens(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;
	static const uint8_t WANT[6] = { 10, 20, 30, 200, 210, 220 };

	arena = decodes_to(GREY_PNG, sizeof(GREY_PNG), 3, 2, &image);
	for (uint32_t i = 0; i < 6; i++) {
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 0], WANT[i]);
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 1], WANT[i]);
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 2], WANT[i]);
		VOE_TEST_CHECK_INT(image.pixels[i * 4 + 3], 0xff);
	}

	voe_base_arena_destroy(arena);
}

// Grey with an alpha channel keeps the alpha it was given.
static void greyscale_with_alpha_keeps_it(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(GREY_ALPHA_PNG, sizeof(GREY_ALPHA_PNG), 2, 1, &image);
	VOE_TEST_CHECK_INT(image.pixels[0], 90);
	VOE_TEST_CHECK_INT(image.pixels[3], 40);
	VOE_TEST_CHECK_INT(image.pixels[4], 91);
	VOE_TEST_CHECK_INT(image.pixels[7], 250);

	voe_base_arena_destroy(arena);
}

// A palette is an indirection and tRNS is a shorter one. The third palette entry
// has no alpha byte, so it must come out opaque rather than reading past tRNS.
static void a_palette_resolves_and_trns_runs_out(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(PALETTE_PNG, sizeof(PALETTE_PNG), 2, 2, &image);

	// index 0 -> red, alpha 255
	VOE_TEST_CHECK_INT(image.pixels[0], 255);
	VOE_TEST_CHECK_INT(image.pixels[1], 0);
	VOE_TEST_CHECK_INT(image.pixels[3], 255);
	// index 1 -> green, alpha 128
	VOE_TEST_CHECK_INT(image.pixels[5], 255);
	VOE_TEST_CHECK_INT(image.pixels[7], 128);
	// index 2 -> blue, past the end of tRNS, so opaque
	VOE_TEST_CHECK_INT(image.pixels[10], 255);
	VOE_TEST_CHECK_INT(image.pixels[11], 255);

	voe_base_arena_destroy(arena);
}

static void rgba_passes_straight_through(void)
{
	voe_assets_image image = { 0 };
	voe_base_arena *arena;

	arena = decodes_to(RGBA_PNG, sizeof(RGBA_PNG), 2, 1, &image);
	for (int i = 0; i < 8; i++)
		VOE_TEST_CHECK_INT(image.pixels[i], i + 1);

	voe_base_arena_destroy(arena);
}

// THE THREE THE CARD NAMED, PLUS THE ONES THAT COST NOTHING ONCE THE HARNESS
// EXISTS. Every length in the file is truncated in turn rather than at one
// arbitrary point: a decoder usually survives losing its tail and falls over
// losing the middle of a chunk header, and only a sweep finds that.
static void a_truncated_file_is_refused(void)
{
	for (size_t size = 0; size < sizeof(RGB_PNG); size++)
		refuses("truncated", RGB_PNG, size, VOE_BASE_ERROR_MALFORMED);
}

static void a_wrong_magic_number_is_refused(void)
{
	uint8_t bytes[sizeof(RGB_PNG)];

	memcpy(bytes, RGB_PNG, sizeof(bytes));
	bytes[1] = 'X';
	refuses("magic", bytes, sizeof(bytes), VOE_BASE_ERROR_MALFORMED);

	// A JPEG handed to the PNG reader is the commonest wrong magic there is.
	static const uint8_t JPEG_START[8] = { 0xff, 0xd8, 0xff, 0xe0,
					       0x00, 0x10, 'J', 'F' };
	refuses("a jpeg", JPEG_START, sizeof(JPEG_START),
		VOE_BASE_ERROR_MALFORMED);
}

// A hundred thousand square is four hundred billion bytes. Refusing on the
// header is the whole point: the allocation must never be attempted. It is a
// limit the engine keeps, so unsupported, and the line names both numbers.
static void an_absurd_declared_size_is_refused(void)
{
	refuses("huge", HUGE_PNG, sizeof(HUGE_PNG), VOE_BASE_ERROR_UNSUPPORTED);
	VOE_TEST_CHECK(said("100000 × 100000"));
	VOE_TEST_CHECK(said("16384"));
}

static void a_corrupt_chunk_is_refused(void)
{
	refuses("bad crc", BAD_CRC_PNG, sizeof(BAD_CRC_PNG),
		VOE_BASE_ERROR_MALFORMED);
}

// The eight bytes a PNG starts with: written by build_png below, and read back
// out of the writer's files rather than trusted.
static const uint8_t PNG_SIGNATURE[8] = { 0x89, 'P', 'N', 'G', 0x0d,
					  0x0a, 0x1a, 0x0a };

static void put_be32(uint8_t *at, uint32_t word)
{
	for (int i = 0; i < 4; i++)
		at[i] = (uint8_t)(word >> (24 - 8 * i));
}

// One chunk at `at`: length, type, data and the CRC over type and data. Its
// size is returned.
static size_t put_chunk(uint8_t *at, const char *type, const uint8_t *data,
			uint32_t count)
{
	put_be32(at, count);
	memcpy(at + 4, type, 4);
	if (count > 0)
		memcpy(at + 8, data, count);
	put_be32(at + 8 + count, voe_assets_png_crc(at + 4, 4 + (size_t)count));
	return 12 + (size_t)count;
}

// A PNG of `scanlines` (filter bytes included) into `out`, its size returned:
// the IDAT is one stored deflate block in a zlib wrapper, so the bytes are the
// test's own and no encoder chose them. `scanlines` is under 256 bytes.
static size_t build_png(uint8_t *out, uint32_t width, uint32_t height,
			uint8_t depth, uint8_t colour_type,
			const uint8_t *scanlines, uint32_t count)
{
	uint8_t header[13] = { 0 };
	uint8_t zlib[2 + 5 + 256 + 4];
	uint32_t a = 1;
	uint32_t b = 0;
	size_t size = sizeof(PNG_SIGNATURE);

	put_be32(header, width);
	put_be32(header + 4, height);
	header[8] = depth;
	header[9] = colour_type;

	zlib[0] = 0x78;
	zlib[1] = 0x01;
	zlib[2] = 0x01; // the last block, stored
	zlib[3] = (uint8_t)count;
	zlib[4] = (uint8_t)(count >> 8);
	zlib[5] = (uint8_t)~count;
	zlib[6] = (uint8_t)(~count >> 8);
	memcpy(zlib + 7, scanlines, count);
	for (uint32_t i = 0; i < count; i++) {
		a = (a + scanlines[i]) % 65521;
		b = (b + a) % 65521;
	}
	put_be32(zlib + 7 + count, (b << 16) | a);

	memcpy(out, PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
	size += put_chunk(out + size, "IHDR", header, sizeof(header));
	size += put_chunk(out + size, "IDAT", zlib, 7 + count + 4);
	size += put_chunk(out + size, "IEND", NULL, 0);
	return size;
}

// Two RGBA pixels at 16 bits, the second through the Sub filter so "left" is
// eight bytes back. Each sample rounds to the nearest 8-bit value (0446).
static void a_16_bit_rgba_png_rounds_to_8_bits(void)
{
	static const uint16_t SAMPLES[8] = { 0x0000, 0xffff, 0x807f, 0x7f80,
					     0x7f80, 0x807f, 0xffff, 0x0000 };
	static const uint8_t WANT[8] = { 0, 255, 128, 127, 127, 128, 255, 0 };
	static uint8_t file[512];
	uint8_t raw[16];
	uint8_t row[17];
	voe_assets_image image = { 0 };
	voe_base_arena *arena;
	size_t size;

	for (int i = 0; i < 8; i++) {
		raw[i * 2] = (uint8_t)(SAMPLES[i] >> 8);
		raw[i * 2 + 1] = (uint8_t)SAMPLES[i];
	}
	row[0] = 1; // Sub
	for (int i = 0; i < 16; i++)
		row[1 + i] = (uint8_t)(raw[i] - (i >= 8 ? raw[i - 8] : 0));

	size = build_png(file, 2, 1, 16, 6, row, sizeof(row));
	arena = decodes_to(file, size, 2, 1, &image);
	for (int i = 0; i < 8; i++)
		VOE_TEST_CHECK_INT(image.pixels[i], WANT[i]);

	voe_base_arena_destroy(arena);
}

// 16-bit grey widens like 8-bit grey: three equal channels, alpha 255.
static void a_16_bit_greyscale_png_widens(void)
{
	static const uint8_t ROW[5] = { 0, 0x12, 0x34, 0xff, 0xff };
	static uint8_t file[512];
	voe_assets_image image = { 0 };
	voe_base_arena *arena;
	size_t size;

	size = build_png(file, 2, 1, 16, 0, ROW, sizeof(ROW));
	arena = decodes_to(file, size, 2, 1, &image);
	// (0x1234 + 128) / 257 = 18.
	VOE_TEST_CHECK_INT(image.pixels[0], 18);
	VOE_TEST_CHECK_INT(image.pixels[1], 18);
	VOE_TEST_CHECK_INT(image.pixels[2], 18);
	VOE_TEST_CHECK_INT(image.pixels[3], 255);
	VOE_TEST_CHECK_INT(image.pixels[4], 255);
	VOE_TEST_CHECK_INT(image.pixels[6], 255);
	VOE_TEST_CHECK_INT(image.pixels[7], 255);

	voe_base_arena_destroy(arena);
}

// WELL-FORMED AND DECLINED IS A DIFFERENT ANSWER FROM BROKEN, AND THE CODE SAYS
// WHICH. Another program would read the first two files perfectly well; the
// third is a palette at 16 bits, which the format forbids.
static void interlaced_and_shallow_are_unsupported_not_malformed(void)
{
	static const uint8_t SHALLOW[2] = { 0, 0x5a };
	static const uint8_t WIDE[5] = { 0, 0, 0, 0, 1 };
	static uint8_t file[512];
	size_t size;

	refuses("interlaced", INTERLACED_PNG, sizeof(INTERLACED_PNG),
		VOE_BASE_ERROR_UNSUPPORTED);
	VOE_TEST_CHECK(said("interlac"));
	size = build_png(file, 2, 1, 4, 0, SHALLOW, sizeof(SHALLOW));
	refuses("4-bit", file, size, VOE_BASE_ERROR_UNSUPPORTED);
	VOE_TEST_CHECK(said("4-bit"));
	size = build_png(file, 2, 1, 16, 3, WIDE, sizeof(WIDE));
	refuses("16-bit palette", file, size, VOE_BASE_ERROR_UNSUPPORTED);
}

static uint32_t be32_at(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
	       ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

// Encode a picture, decode it straight back, and compare every byte. The arena
// is destroyed here because nothing outlives the comparison.
static void round_trips(uint32_t width, uint32_t height, uint8_t *pixels)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_image image = { width, height, pixels };
	voe_assets_image back = { 0 };
	voe_assets_bytes file = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_assets_png_encode(arena, image, &file, &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_OK);
	VOE_TEST_CHECK(file.count > 0);

	// The signature and IHDR, straight out of the bytes. A picture that
	// round-trips through our own decoder with a wrong header still fails
	// everywhere else.
	VOE_TEST_CHECK(file.count > 33);
	VOE_TEST_CHECK(memcmp(file.bytes, PNG_SIGNATURE,
			      sizeof(PNG_SIGNATURE)) == 0);
	VOE_TEST_CHECK_INT((int)be32_at(file.bytes + 8), 13);
	VOE_TEST_CHECK(memcmp(file.bytes + 12, "IHDR", 4) == 0);
	VOE_TEST_CHECK_INT((int)be32_at(file.bytes + 16), (int)width);
	VOE_TEST_CHECK_INT((int)be32_at(file.bytes + 20), (int)height);
	VOE_TEST_CHECK_INT(file.bytes[24], 8);  // bit depth
	VOE_TEST_CHECK_INT(file.bytes[25], 6);  // colour type: RGBA
	VOE_TEST_CHECK_INT(file.bytes[26], 0);  // compression
	VOE_TEST_CHECK_INT(file.bytes[27], 0);  // filter method
	VOE_TEST_CHECK_INT(file.bytes[28], 0);  // interlace

	VOE_TEST_CHECK(voe_assets_png_decode(file.bytes, file.count, arena,
					     &back, &error));
	VOE_TEST_CHECK_INT((int)back.width, (int)width);
	VOE_TEST_CHECK_INT((int)back.height, (int)height);

	for (size_t i = 0; i < (size_t)width * height * 4; i++)
		VOE_TEST_CHECK_INT(back.pixels[i], pixels[i]);

	voe_base_arena_destroy(arena);
}

// Four different corners, so a picture written sideways or upside down fails.
static void a_picture_survives_the_round_trip(void)
{
	uint8_t pixels[4 * 3 * 4];

	for (size_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i * 7 + 3);

	// The corners, by hand, so the failure names a corner.
	memcpy(pixels + 0, (uint8_t[]){ 255, 0, 0, 255 }, 4);
	memcpy(pixels + 3 * 4, (uint8_t[]){ 0, 255, 0, 255 }, 4);
	memcpy(pixels + 8 * 4, (uint8_t[]){ 0, 0, 255, 255 }, 4);
	memcpy(pixels + 11 * 4, (uint8_t[]){ 255, 255, 0, 255 }, 4);

	round_trips(4, 3, pixels);
}

static void one_pixel_is_a_picture(void)
{
	uint8_t pixels[4] = { 17, 34, 51, 68 };

	round_trips(1, 1, pixels);
}

// AN ODD WIDTH IS WHAT CATCHES A STRIDE WORKED OUT FROM THE WRONG NUMBER. 257
// pixels is 1028 bytes of colour and a 1029-byte filtered row, and every
// off-by-one in either direction shifts the picture along by a pixel a row.
static void an_odd_width_keeps_its_rows_apart(void)
{
	static uint8_t pixels[257 * 3 * 4];

	for (size_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i * 31 + i / 257);

	round_trips(257, 3, pixels);
}

// Alpha is written as it arrives — nothing premultiplies, nothing forces opaque.
static void alpha_comes_back_unchanged(void)
{
	uint8_t pixels[3 * 1 * 4] = {
		200, 100, 50, 0,   //
		200, 100, 50, 128, //
		200, 100, 50, 255, //
	};

	round_trips(3, 1, pixels);
}

// The one returned failure: a picture whose bytes could not be counted. Nothing
// is allocated and nothing is touched on the way out, which is why `pixels` can
// be a single byte.
static void a_picture_too_large_to_measure_is_refused(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	uint8_t one = 0;
	voe_assets_image image = { 0xffffffffu, 0xffffffffu, &one };
	voe_assets_bytes file = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_assets_png_encode(arena, image, &file, &error));
	VOE_TEST_CHECK_INT((int)error, (int)VOE_BASE_ERROR_REFUSED);

	voe_base_arena_destroy(arena);
}

int main(void)
{
	truecolour_with_every_filter();
	a_stored_deflate_block_decodes();
	greyscale_widens();
	greyscale_with_alpha_keeps_it();
	a_palette_resolves_and_trns_runs_out();
	rgba_passes_straight_through();
	a_truncated_file_is_refused();
	a_wrong_magic_number_is_refused();
	an_absurd_declared_size_is_refused();
	a_corrupt_chunk_is_refused();
	a_16_bit_rgba_png_rounds_to_8_bits();
	a_16_bit_greyscale_png_widens();
	interlaced_and_shallow_are_unsupported_not_malformed();
	a_picture_survives_the_round_trip();
	one_pixel_is_a_picture();
	an_odd_width_keeps_its_rows_apart();
	alpha_comes_back_unchanged();
	a_picture_too_large_to_measure_is_refused();
	return voe_test_result();
}
