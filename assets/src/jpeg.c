// The JPEG reader. Baseline only, and everything else is refused out loud — see
// include/assets/image.h for what a caller gets.
//
// IT IS THE BIGGER OF THE TWO DECODERS AND IT IS NOT CLOSE. PNG is one
// compressed stream and a per-row filter; a JPEG is a container of tables, then
// Huffman-coded coefficients, then a dequantise, then an inverse cosine
// transform, then a chroma upsample, then a colour conversion — six stages, each
// of which can be wrong on its own and produce a picture that merely looks odd
// rather than failing.
//
// PROGRESSIVE JPEG IS A DIFFERENT DECODER AND IT IS REFUSED RATHER THAN
// ATTEMPTED. A progressive file sends the coefficients in successive passes that
// refine each other, so nothing can be reconstructed until the last of them
// arrives and the whole scan loop below is the wrong shape for it. Decoding one
// with this code would produce a corrupt image, which is worse than saying no —
// so SOF2 is VOE_BASE_ERROR_UNSUPPORTED and the file is untouched.
//
// EVERY LENGTH, COUNT AND INDEX IN THE FILE IS HOSTILE. A JPEG names its tables
// by number, its scan components by id and its coefficients by a run length that
// skips forward through a block — so a corrupt file's natural attack is an index
// that walks off the end of a table, a plane or a 64-entry block. Each of those
// is bounded at the point of use below, and none of them is trusted because the
// header looked reasonable.
//
// THERE IS NO RECURSION HERE EITHER. Markers are a flat list with a length on
// each, the same as PNG's chunks, so ADR-0043's explicit-stack rule has nothing
// to bite on. It is named because it was considered.
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

// Three is a colour JPEG and one is greyscale. Four is CMYK, which is a
// different colour conversion and has no caller.
#define MAX_COMPONENTS 3

// The largest sampling factor JPEG allows. It bounds how many blocks one MCU
// holds, which is what bounds the loop that decodes them.
#define MAX_SAMPLING 4

// Zigzag order to the natural position in an 8x8 block. Both the coefficients
// and the quantisation table arrive in this order; the table below is what puts
// them back.
static const uint8_t ZIGZAG[64] = {
	0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

// The inverse cosine transform, as a table rather than as calls to cosf.
//
// COS_TABLE[x][u] IS 0.5 * C(u) * cos((2x+1) u pi / 16), WHICH IS THE WHOLE OF
// THE ONE-DIMENSIONAL TRANSFORM. Applying it down the rows and then down the
// columns gives the two-dimensional one, and the two halves multiply to the
// quarter the standard asks for. It is written out rather than computed at
// startup so that this folder needs no maths library and so that the numbers are
// the same on both platforms whatever the compiler does with cosf.
static const float COS_TABLE[8][8] = {
	{ 0.353553391f, 0.490392640f, 0.461939766f, 0.415734806f, 0.353553391f, 0.277785117f, 0.191341716f, 0.097545161f },
	{ 0.353553391f, 0.415734806f, 0.191341716f, -0.097545161f, -0.353553391f, -0.490392640f, -0.461939766f, -0.277785117f },
	{ 0.353553391f, 0.277785117f, -0.191341716f, -0.490392640f, -0.353553391f, 0.097545161f, 0.461939766f, 0.415734806f },
	{ 0.353553391f, 0.097545161f, -0.461939766f, -0.277785117f, 0.353553391f, 0.415734806f, -0.191341716f, -0.490392640f },
	{ 0.353553391f, -0.097545161f, -0.461939766f, 0.277785117f, 0.353553391f, -0.415734806f, -0.191341716f, 0.490392640f },
	{ 0.353553391f, -0.277785117f, -0.191341716f, 0.490392640f, -0.353553391f, -0.097545161f, 0.461939766f, -0.415734806f },
	{ 0.353553391f, -0.415734806f, 0.191341716f, 0.097545161f, -0.353553391f, 0.490392640f, -0.461939766f, 0.277785117f },
	{ 0.353553391f, -0.490392640f, 0.461939766f, -0.415734806f, 0.353553391f, -0.277785117f, 0.191341716f, -0.097545161f },
};

// A Huffman table in the form the standard's DECODE procedure wants. Built from
// the sixteen counts and the values that follow them.
struct huffman {
	int32_t mincode[17];
	int32_t maxcode[18];
	int32_t valptr[17];
	uint8_t values[256];
	bool present;
};

struct component {
	uint8_t id;
	uint8_t h;
	uint8_t v;
	uint8_t quant;
	uint8_t dc_table;
	uint8_t ac_table;
	int32_t predictor;

	// The component's own plane, in blocks of eight, which is what the MCU
	// grid rounds up to. Chroma planes are smaller than the picture and are
	// stretched over it when the colours are put together.
	uint8_t *plane;
	uint32_t plane_width;
	uint32_t plane_height;
};

struct decoder {
	const uint8_t *bytes;
	size_t size;
	size_t next;

	// The entropy-coded bit stream, which is not the same cursor as `next`:
	// it stops at the first marker rather than running through it.
	uint32_t value;
	int count;
	bool overrun;

	// RUNNING OUT OF FILE AND REACHING A MARKER BOTH STOP THE BIT STREAM,
	// AND ONLY ONE OF THEM IS NORMAL. A scan ends at a marker, so `overrun`
	// on its own says nothing is wrong. `truncated` says the bytes simply
	// stopped, which is a damaged file — and without the distinction a JPEG
	// cut in half decodes to a picture whose bottom is whatever zero bits
	// happen to mean.
	bool truncated;

	uint16_t quant[4][64];
	bool quant_present[4];
	struct huffman dc[4];
	struct huffman ac[4];

	struct component components[MAX_COMPONENTS];
	uint32_t component_count;

	uint32_t width;
	uint32_t height;
	uint32_t hmax;
	uint32_t vmax;
	uint32_t mcus_x;
	uint32_t mcus_y;
	uint32_t restart_interval;
};

static bool fail(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
	return false;
}

// ------------------------------------------------------------ the bit stream

// One bit, most significant first, which is the opposite of DEFLATE's order.
//
// 0xFF IS SPECIAL AND THAT IS THE WHOLE AWKWARDNESS OF THIS FORMAT'S ENTROPY
// CODING. A literal 0xFF in the coded data is written as 0xFF 0x00, because
// every real marker also begins 0xFF — so a reader that did not un-stuff would
// see a marker in the middle of the picture. An 0xFF followed by anything else
// is a marker, which means the scan has ended: the stream reports zeroes from
// then on and the caller stops at the end of the current MCU.
static uint32_t take_bit(struct decoder *jpeg)
{
	if (jpeg->count == 0) {
		uint8_t byte;

		if (jpeg->next >= jpeg->size) {
			jpeg->overrun = true;
			jpeg->truncated = true;
			return 0;
		}

		byte = jpeg->bytes[jpeg->next++];
		if (byte == 0xff) {
			uint8_t next;

			if (jpeg->next >= jpeg->size) {
				// An 0xFF as the last byte in the file: the
				// marker it begins was cut off.
				jpeg->overrun = true;
				jpeg->truncated = true;
				return 0;
			}
			next = jpeg->bytes[jpeg->next];

			if (next == 0x00) {
				jpeg->next++;
			} else {
				// A marker. Step back onto the 0xFF so that the
				// scan loop can see which one it was.
				jpeg->next--;
				jpeg->overrun = true;
				return 0;
			}
		}

		jpeg->value = byte;
		jpeg->count = 8;
	}

	jpeg->count--;
	return (jpeg->value >> jpeg->count) & 1u;
}

static int32_t take_bits(struct decoder *jpeg, int wanted)
{
	int32_t value = 0;

	for (int i = 0; i < wanted; i++)
		value = (value << 1) | (int32_t)take_bit(jpeg);

	return value;
}

// The standard's EXTEND: a `length`-bit magnitude whose top bit clear means the
// value was negative. Without it every coefficient comes out positive and the
// picture is a bright mess.
static int32_t extend(int32_t value, int length)
{
	if (length == 0)
		return 0;

	return value < (1 << (length - 1)) ? value - (1 << length) + 1 : value;
}

// The standard's DECODE. Walks one bit at a time until the accumulated code
// falls inside the range recorded for that length. -1 for a code no length
// contains, which an exhausted stream also produces.
static int32_t huffman_decode(struct decoder *jpeg, const struct huffman *table)
{
	int32_t code = (int32_t)take_bit(jpeg);
	int length = 1;
	int32_t index;

	while (code > table->maxcode[length]) {
		if (++length > 16)
			return -1;
		code = (code << 1) | (int32_t)take_bit(jpeg);
	}

	index = table->valptr[length] + code - table->mincode[length];
	if (index < 0 || index > 255)
		return -1;

	return table->values[index];
}

// ----------------------------------------------------------- the 8x8 machinery

// Coefficients to samples, rows then columns, and the level shift at the end.
static void inverse_dct(const int32_t *coefficients, uint8_t *out,
			uint32_t stride)
{
	float rows[64];
	float columns[64];

	for (uint32_t y = 0; y < 8; y++) {
		for (uint32_t x = 0; x < 8; x++) {
			float sum = 0.0f;

			for (uint32_t u = 0; u < 8; u++)
				sum += COS_TABLE[x][u] *
				       (float)coefficients[y * 8 + u];

			rows[y * 8 + x] = sum;
		}
	}

	for (uint32_t x = 0; x < 8; x++) {
		for (uint32_t y = 0; y < 8; y++) {
			float sum = 0.0f;

			for (uint32_t v = 0; v < 8; v++)
				sum += COS_TABLE[y][v] * rows[v * 8 + x];

			columns[y * 8 + x] = sum;
		}
	}

	// PLUS 128 AND CLAMPED, BECAUSE THE ENCODER SUBTRACTED 128 BEFORE
	// TRANSFORMING. The clamp is not belt and braces: a lossy transform
	// genuinely produces values a little outside 0..255 at sharp edges, and
	// letting those wrap is the classic source of bright specks on a dark
	// border.
	for (uint32_t y = 0; y < 8; y++) {
		for (uint32_t x = 0; x < 8; x++) {
			float value = columns[y * 8 + x] + 128.0f;

			if (value < 0.0f)
				value = 0.0f;
			if (value > 255.0f)
				value = 255.0f;

			out[y * stride + x] = (uint8_t)(value + 0.5f);
		}
	}
}

// One block: the DC difference, then the run-length-coded AC coefficients,
// dequantised on the way in and written into the component's plane.
[[nodiscard]] static bool decode_block(struct decoder *jpeg,
				       struct component *component,
				       uint32_t block_x, uint32_t block_y)
{
	int32_t coefficients[64] = { 0 };
	const uint16_t *quant = jpeg->quant[component->quant];
	const struct huffman *dc = &jpeg->dc[component->dc_table];
	const struct huffman *ac = &jpeg->ac[component->ac_table];
	int32_t length;
	uint32_t k = 1;

	length = huffman_decode(jpeg, dc);
	if (length < 0 || length > 16)
		return false;

	component->predictor += extend(take_bits(jpeg, (int)length),
				       (int)length);
	coefficients[0] = component->predictor * (int32_t)quant[0];

	while (k < 64) {
		int32_t rs = huffman_decode(jpeg, ac);
		int32_t run;
		int32_t size;

		if (rs < 0)
			return false;

		run = rs >> 4;
		size = rs & 15;

		if (size == 0) {
			// 0x00 ends the block; 0xF0 is a run of sixteen zeroes.
			if (run != 15)
				break;
			k += 16;
			continue;
		}

		k += (uint32_t)run;
		// THE RUN LENGTH IS THE INDEX THAT WALKS OFF THE BLOCK. A
		// corrupt file's easiest attack on this decoder is a run that
		// takes k past 63, and this is where that stops.
		if (k > 63)
			return false;

		coefficients[ZIGZAG[k]] =
			extend(take_bits(jpeg, (int)size), (int)size) *
			(int32_t)quant[k];
		k++;
	}

	inverse_dct(coefficients,
		    component->plane + (size_t)block_y * 8 *
					       component->plane_width +
			    (size_t)block_x * 8,
		    component->plane_width);
	return true;
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
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	// Eight bits a sample. Twelve is legal JPEG and a different set of
	// tables, and nothing has asked for one.
	if (data[0] != 8)
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	jpeg->height = ((uint32_t)data[1] << 8) | data[2];
	jpeg->width = ((uint32_t)data[3] << 8) | data[4];
	count = data[5];

	if (jpeg->width == 0 || jpeg->height == 0 ||
	    jpeg->width > VOE_ASSETS_IMAGE_MAX_SIDE ||
	    jpeg->height > VOE_ASSETS_IMAGE_MAX_SIDE)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	// One is greyscale and three is colour. Four is CMYK, which is
	// well-formed and needs a colour conversion this decoder does not have.
	if (count != 1 && count != 3)
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);
	if (length - 6 < count * 3)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

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
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		if (component->h > jpeg->hmax)
			jpeg->hmax = component->h;
		if (component->v > jpeg->vmax)
			jpeg->vmax = component->v;
	}

	jpeg->mcus_x = (jpeg->width + jpeg->hmax * 8 - 1) / (jpeg->hmax * 8);
	jpeg->mcus_y = (jpeg->height + jpeg->vmax * 8 - 1) / (jpeg->vmax * 8);
	return true;
}

// The scan header: which components are in it, and with which tables. Baseline
// has exactly one scan and it carries every component.
[[nodiscard]] static bool read_scan(struct decoder *jpeg, const uint8_t *data,
				    uint32_t length)
{
	uint32_t count;

	if (length < 1)
		return false;

	count = data[0];
	if (count != jpeg->component_count || length < 1 + count * 2 + 3)
		return false;

	for (uint32_t i = 0; i < count; i++) {
		uint8_t id = data[1 + i * 2];
		uint8_t tables = data[2 + i * 2];
		struct component *found = NULL;

		for (uint32_t c = 0; c < jpeg->component_count; c++) {
			if (jpeg->components[c].id == id) {
				found = &jpeg->components[c];
				break;
			}
		}

		// A scan naming a component the frame never declared.
		if (found == NULL)
			return false;

		found->dc_table = tables >> 4;
		found->ac_table = tables & 15;
		if (found->dc_table >= 4 || found->ac_table >= 4)
			return false;
		if (!jpeg->dc[found->dc_table].present ||
		    !jpeg->ac[found->ac_table].present)
			return false;
	}

	return true;
}

// Every MCU, in order, each holding h*v blocks of each component.
[[nodiscard]] static bool decode_scan(struct decoder *jpeg)
{
	uint32_t since_restart = 0;

	for (uint32_t my = 0; my < jpeg->mcus_y; my++) {
		for (uint32_t mx = 0; mx < jpeg->mcus_x; mx++) {
			for (uint32_t c = 0; c < jpeg->component_count; c++) {
				struct component *component =
					&jpeg->components[c];

				for (uint32_t by = 0; by < component->v; by++) {
					for (uint32_t bx = 0;
					     bx < component->h; bx++) {
						if (!decode_block(
							    jpeg, component,
							    mx * component->h + bx,
							    my * component->v + by))
							return false;
					}
				}
			}

			// RESTART MARKERS, WHICH EXIST SO THAT A DAMAGED FILE
			// CAN RESYNCHRONISE. At every interval the bit stream
			// is byte-aligned, an RSTn marker sits in the data, and
			// every DC predictor goes back to zero. Skipping the
			// marker without resetting the predictors gives an
			// image whose brightness drifts in bands, which looks
			// like a fault in the picture rather than in the reader.
			if (jpeg->restart_interval != 0) {
				if (++since_restart == jpeg->restart_interval) {
					since_restart = 0;

					jpeg->count = 0;
					jpeg->overrun = false;

					if (jpeg->next + 1 < jpeg->size &&
					    jpeg->bytes[jpeg->next] == 0xff &&
					    jpeg->bytes[jpeg->next + 1] >= 0xd0 &&
					    jpeg->bytes[jpeg->next + 1] <= 0xd7)
						jpeg->next += 2;

					for (uint32_t c = 0;
					     c < jpeg->component_count; c++)
						jpeg->components[c].predictor = 0;
				}
			}
		}
	}

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
		return fail(error, VOE_BASE_ERROR_MALFORMED);

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
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		marker = bytes[jpeg.next++];
		if (marker == M_EOI)
			break;
		// Standalone markers carry no length.
		if (marker >= 0xd0 && marker <= 0xd7)
			continue;

		if (jpeg.next + 2 > size)
			return fail(error, VOE_BASE_ERROR_MALFORMED);
		length = ((uint32_t)bytes[jpeg.next] << 8) |
			 bytes[jpeg.next + 1];
		if (length < 2 || length - 2 > size - jpeg.next - 2)
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		data = bytes + jpeg.next + 2;
		length -= 2;

		switch (marker) {
		case M_SOF0:
			if (seen_frame)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			if (!read_frame(&jpeg, data, length, error))
				return false;
			seen_frame = true;
			break;

		// PROGRESSIVE, AND THE ONE REFUSAL THE CARD NAMED. Well-formed,
		// widely produced, and a different decoder — see the top of
		// this file.
		case M_SOF2:
			return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

		case M_DQT:
			if (!read_quant_tables(&jpeg, data, length))
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			break;

		case M_DHT:
			if (!read_huffman_tables(&jpeg, data, length))
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			break;

		case M_DRI:
			if (length < 2)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			jpeg.restart_interval =
				((uint32_t)data[0] << 8) | data[1];
			break;

		case M_SOS: {
			if (!seen_frame)
				return fail(error, VOE_BASE_ERROR_MALFORMED);
			if (!read_scan(&jpeg, data, length))
				return fail(error, VOE_BASE_ERROR_MALFORMED);

			for (uint32_t c = 0; c < jpeg.component_count; c++) {
				struct component *component =
					&jpeg.components[c];

				if (!jpeg.quant_present[component->quant])
					return fail(error,
						    VOE_BASE_ERROR_MALFORMED);

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

			if (!decode_scan(&jpeg))
				return fail(error, VOE_BASE_ERROR_MALFORMED);

			// The coded data ran out rather than ending at a
			// marker.
			if (jpeg.truncated)
				return fail(error, VOE_BASE_ERROR_MALFORMED);

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
					return fail(error,
						    VOE_BASE_ERROR_MALFORMED);
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
	return fail(error, VOE_BASE_ERROR_MALFORMED);
}
