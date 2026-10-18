// The JPEG reader. Baseline only, and everything else is refused out loud — see
// include/assets/image.h for what a caller gets. This file reads the markers and
// their tables, then upsamples the chroma and converts the colours; the scan
// between them, the entropy-coded data, is jpeg_scan.c's, and jpeg_read.h is what
// the two share.
//
// IT IS THE BIGGER OF THE TWO DECODERS AND IT IS NOT CLOSE. PNG is one
// compressed stream and a per-row filter; a JPEG is six stages, each of which
// can be wrong on its own — jpeg_read.h names them.
//
// PROGRESSIVE JPEG IS A DIFFERENT DECODER AND IT IS REFUSED RATHER THAN
// ATTEMPTED. A progressive file sends the coefficients in successive passes that
// refine each other, so nothing can be reconstructed until the last of them
// arrives and the whole scan loop in jpeg_scan.c is the wrong shape for it.
// Decoding one with this code would produce a corrupt image, which is worse than
// saying no — so SOF2 is VOE_BASE_ERROR_UNSUPPORTED and the file is untouched.
#include "jpeg_read.h"

#include <assets/image.h>

#include <base/assert.h>

#include <string.h>

// Only the markers this decoder acts on. Everything else with a length is
// skipped by that length, which is what makes an unknown APPn or COM harmless.
#define M_SOF0 0xc0
#define M_SOF2 0xc2
#define M_DHT 0xc4
#define M_SOI 0xd8
#define M_EOI 0xd9
#define M_SOS 0xda
#define M_DQT 0xdb
#define M_DRI 0xdd

// The largest sampling factor JPEG allows. It bounds how many blocks one MCU
// holds, which is what bounds the loop that decodes them.
#define MAX_SAMPLING 4

bool voe_assets_jpeg_fail(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
	return false;
}

// ---------------------------------------------------------------- the markers

[[nodiscard]] static bool read_quant_tables(struct decoder *jpeg,
					    const uint8_t *data, uint32_t length)
{
	uint32_t offset = 0;

	while (offset < length) {
		uint8_t header = data[offset++];
		uint32_t precision = header >> 4;
		uint32_t id = header & 15;
		uint32_t entries = precision ? 128 : 64;

		if (id >= 4 || precision > 1)
			return false;
		if (entries > length - offset)
			return false;

		for (uint32_t i = 0; i < 64; i++)
			jpeg->quant[id][i] =
				precision ? (uint16_t)((data[offset + i * 2] << 8) |
						       data[offset + i * 2 + 1])
					  : data[offset + i];

		jpeg->quant_present[id] = true;
		offset += entries;
	}

	return true;
}

[[nodiscard]] static bool read_huffman_tables(struct decoder *jpeg,
					      const uint8_t *data,
					      uint32_t length)
{
	uint32_t offset = 0;

	while (offset < length) {
		uint8_t header;
		uint32_t class;
		uint32_t id;
		struct huffman *table;
		uint32_t total = 0;
		int32_t code = 0;
		int32_t k = 0;

		if (length - offset < 17)
			return false;

		header = data[offset++];
		class = header >> 4;
		id = header & 15;
		if (class > 1 || id >= 4)
			return false;

		table = class ? &jpeg->ac[id] : &jpeg->dc[id];

		for (uint32_t l = 1; l <= 16; l++) {
			uint32_t count = data[offset + l - 1];

			table->valptr[l] = k;
			table->mincode[l] = code;
			code += (int32_t)count;
			k += (int32_t)count;
			// -1 for a length with no codes, so that the walk in
			// huffman_decode always moves on from it.
			table->maxcode[l] = count ? code - 1 : -1;
			code <<= 1;
			total += count;
		}
		table->maxcode[17] = 0x7fffffff;
		offset += 16;

		if (total > 256 || total > length - offset)
			return false;

		memcpy(table->values, data + offset, total);
		table->present = true;
		offset += total;
	}

	return true;
}

[[nodiscard]] static bool read_frame(struct decoder *jpeg, const uint8_t *data,
				     uint32_t length, voe_base_error *error)
{
	uint32_t count;

	if (length < 6)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

	// Eight bits a sample. Twelve is legal JPEG and a different set of
	// tables, and nothing has asked for one.
	if (data[0] != 8)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	jpeg->height = ((uint32_t)data[1] << 8) | data[2];
	jpeg->width = ((uint32_t)data[3] << 8) | data[4];
	count = data[5];

	if (jpeg->width == 0 || jpeg->height == 0 ||
	    jpeg->width > VOE_ASSETS_IMAGE_MAX_SIDE ||
	    jpeg->height > VOE_ASSETS_IMAGE_MAX_SIDE)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

	// One is greyscale and three is colour. Four is CMYK, which is
	// well-formed and needs a colour conversion this decoder does not have.
	if (count != 1 && count != 3)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_UNSUPPORTED);
	if (length - 6 < count * 3)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

	jpeg->component_count = count;
	jpeg->hmax = 1;
	jpeg->vmax = 1;

	for (uint32_t i = 0; i < count; i++) {
		struct component *component = &jpeg->components[i];
		const uint8_t *entry = data + 6 + i * 3;

		component->id = entry[0];
		component->h = entry[1] >> 4;
		component->v = entry[1] & 15;
		component->quant = entry[2];

		if (component->h < 1 || component->h > MAX_SAMPLING ||
		    component->v < 1 || component->v > MAX_SAMPLING ||
		    component->quant >= 4)
			return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

		if (component->h > jpeg->hmax)
			jpeg->hmax = component->h;
		if (component->v > jpeg->vmax)
			jpeg->vmax = component->v;
	}

	jpeg->mcus_x = (jpeg->width + jpeg->hmax * 8 - 1) / (jpeg->hmax * 8);
	jpeg->mcus_y = (jpeg->height + jpeg->vmax * 8 - 1) / (jpeg->vmax * 8);
	return true;
}

// ------------------------------------------------------------ colour and out

static uint8_t clamp_byte(int32_t value)
{
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return (uint8_t)value;
}

// YCbCr to RGB, and the chroma stretch that goes with it.
//
// NEAREST NEIGHBOUR AND NOT A SMOOTH UPSAMPLE. A colour JPEG usually stores its
// two chroma planes at half the width and half the height, so every one of their
// samples covers four pixels; repeating it is what every decoder does at this
// level of effort, and the difference from a bilinear stretch shows only on hard
// colour edges. The card that cares is the card that changes this, with a
// picture to point at.
static void to_rgba(struct decoder *jpeg, uint8_t *pixels)
{
	const struct component *y_plane = &jpeg->components[0];

	for (uint32_t y = 0; y < jpeg->height; y++) {
		for (uint32_t x = 0; x < jpeg->width; x++) {
			uint8_t *pixel = pixels +
					 ((size_t)y * jpeg->width + x) * 4;
			uint32_t sy = y * y_plane->v / jpeg->vmax;
			uint32_t sx = x * y_plane->h / jpeg->hmax;
			int32_t luma = y_plane->plane[(size_t)sy *
							      y_plane->plane_width +
						      sx];

			if (jpeg->component_count == 1) {
				pixel[0] = pixel[1] = pixel[2] =
					clamp_byte(luma);
				pixel[3] = 0xff;
				continue;
			}

			{
				const struct component *cb_plane =
					&jpeg->components[1];
				const struct component *cr_plane =
					&jpeg->components[2];
				uint32_t by = y * cb_plane->v / jpeg->vmax;
				uint32_t bx = x * cb_plane->h / jpeg->hmax;
				uint32_t ry = y * cr_plane->v / jpeg->vmax;
				uint32_t rx = x * cr_plane->h / jpeg->hmax;
				int32_t cb =
					(int32_t)cb_plane->plane[(size_t)by *
									 cb_plane->plane_width +
								 bx] -
					128;
				int32_t cr =
					(int32_t)cr_plane->plane[(size_t)ry *
									 cr_plane->plane_width +
								 rx] -
					128;

				// The JFIF conversion, in fixed point with
				// sixteen fractional bits so that the result
				// does not depend on how a compiler rounds a
				// float.
				pixel[0] = clamp_byte(luma + ((91881 * cr) >> 16));
				pixel[1] = clamp_byte(
					luma - ((22554 * cb + 46802 * cr) >> 16));
				pixel[2] = clamp_byte(luma + ((116130 * cb) >> 16));
				pixel[3] = 0xff;
			}
		}
	}
}

bool voe_assets_jpeg_decode(const uint8_t *bytes, size_t size,
			    voe_base_arena *arena, voe_assets_image *image,
			    voe_base_error *error)
{
	struct decoder jpeg = { .bytes = bytes, .size = size };
	bool seen_frame = false;
	uint8_t *pixels;

	VOE_BASE_DEBUG_ASSERT(bytes != NULL, "decoding a JPEG from nothing");
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "decoding a JPEG with no arena");
	VOE_BASE_DEBUG_ASSERT(image != NULL, "decoding a JPEG into nothing");

	if (size < 2 || bytes[0] != 0xff || bytes[1] != M_SOI)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

	jpeg.next = 2;

	for (;;) {
		uint8_t marker;
		uint32_t length;
		const uint8_t *data;

		// Markers may be preceded by any number of fill bytes.
		while (jpeg.next < size && bytes[jpeg.next] != 0xff)
			jpeg.next++;
		while (jpeg.next < size && bytes[jpeg.next] == 0xff)
			jpeg.next++;
		if (jpeg.next >= size)
			return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

		marker = bytes[jpeg.next++];
		if (marker == M_EOI)
			break;
		// Standalone markers carry no length.
		if (marker >= 0xd0 && marker <= 0xd7)
			continue;

		if (jpeg.next + 2 > size)
			return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
		length = ((uint32_t)bytes[jpeg.next] << 8) |
			 bytes[jpeg.next + 1];
		if (length < 2 || length - 2 > size - jpeg.next - 2)
			return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

		data = bytes + jpeg.next + 2;
		length -= 2;

		switch (marker) {
		case M_SOF0:
			if (seen_frame)
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
			if (!read_frame(&jpeg, data, length, error))
				return false;
			seen_frame = true;
			break;

		// PROGRESSIVE, AND THE ONE REFUSAL THE CARD NAMED. Well-formed,
		// widely produced, and a different decoder — see the top of
		// this file.
		case M_SOF2:
			return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_UNSUPPORTED);

		case M_DQT:
			if (!read_quant_tables(&jpeg, data, length))
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
			break;

		case M_DHT:
			if (!read_huffman_tables(&jpeg, data, length))
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
			break;

		case M_DRI:
			if (length < 2)
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
			jpeg.restart_interval =
				((uint32_t)data[0] << 8) | data[1];
			break;

		case M_SOS: {
			if (!seen_frame)
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
			if (!voe_assets_jpeg_read_scan(&jpeg, data, length))
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

			for (uint32_t c = 0; c < jpeg.component_count; c++) {
				struct component *component =
					&jpeg.components[c];

				if (!jpeg.quant_present[component->quant])
					return voe_assets_jpeg_fail(
						error, VOE_BASE_ERROR_MALFORMED);

				component->plane_width =
					jpeg.mcus_x * component->h * 8;
				component->plane_height =
					jpeg.mcus_y * component->v * 8;
				component->plane = voe_base_arena_push(
					arena, (size_t)component->plane_width *
						       component->plane_height);
			}

			// The entropy-coded data starts straight after the scan
			// header and runs to the next marker, which is why this
			// is the one marker whose payload is not skipped.
			jpeg.next = (size_t)(data - bytes) + length;
			jpeg.count = 0;

			if (!voe_assets_jpeg_decode_scan(&jpeg))
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

			// The coded data ran out rather than ending at a
			// marker.
			if (jpeg.truncated)
				return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);

			// AND THE FILE MUST SAY IT ENDED, WHICH IS THE OTHER
			// HALF OF THE SAME CHECK. Cutting the last bytes off a
			// JPEG can leave every MCU this decoder reads intact,
			// so the coded data never runs out and nothing above
			// notices. EOI is the only thing in the format that
			// says "that was all of it" — the same role IEND plays
			// in a PNG. It cannot be confused with picture data:
			// inside a scan every 0xFF is followed by 0x00 or by a
			// restart marker, never by 0xD9.
			{
				bool seen_end = false;

				while (jpeg.next + 1 < size) {
					uint8_t lead = bytes[jpeg.next];
					uint8_t follow = bytes[jpeg.next + 1];

					if (lead == 0xff && follow != 0x00 &&
					    !(follow >= 0xd0 && follow <= 0xd7)) {
						seen_end = follow == M_EOI;
						break;
					}
					jpeg.next++;
				}

				if (!seen_end)
					return voe_assets_jpeg_fail(
						error, VOE_BASE_ERROR_MALFORMED);
			}

			pixels = voe_base_arena_push(
				arena, (size_t)jpeg.width * jpeg.height * 4);
			to_rgba(&jpeg, pixels);

			image->width = jpeg.width;
			image->height = jpeg.height;
			image->pixels = pixels;
			return true;
		}

		default:
			break;
		}

		jpeg.next = (size_t)(data - bytes) + length;
	}

	// Ran out of file without ever reaching a scan.
	return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED);
}
