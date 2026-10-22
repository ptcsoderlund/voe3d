// The JPEG reader's entropy-coded data: the scan header, then every MCU in
// order, each block Huffman-decoded, dequantised and inverse-transformed into
// its component's plane. jpeg.c reads the markers and calls the two functions
// jpeg_read.h names; nothing here reads a marker but the restart markers.
//
// EVERY LENGTH, COUNT AND INDEX IN THE FILE IS HOSTILE. A JPEG names its tables
// by number, its scan components by id and its coefficients by a run length that
// skips forward through a block — so a corrupt file's natural attack is an index
// that walks off the end of a table, a plane or a 64-entry block. Each of those
// is bounded at the point of use: table numbers and component ids in the scan
// header below and in jpeg.c's table and frame readers, the run length in
// decode_block. None of them is trusted because the header looked reasonable.
//
// THERE IS NO RECURSION HERE EITHER. Markers are a flat list with a length on
// each, the same as PNG's chunks, and the scan is fixed loops over the MCU grid,
// so ADR-0043's explicit-stack rule has nothing to bite on. It is named because
// it was considered.
#include "jpeg_read.h"

#include <inttypes.h>

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

// A block that would not decode, said as damage. A stream that ran out of file
// is said as that, whatever code it happened to make, since it is the cause.
static bool damaged_block(const struct decoder *jpeg, voe_base_error *error,
			  const char *what)
{
	return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED,
				    DAMAGED "(JPEG: %s)",
				    jpeg->truncated ? "the scan data is cut short"
						    : what);
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
				       uint32_t block_x, uint32_t block_y,
				       voe_base_error *error)
{
	int32_t coefficients[64] = { 0 };
	const uint16_t *quant = jpeg->quant[component->quant];
	const struct huffman *dc = &jpeg->dc[component->dc_table];
	const struct huffman *ac = &jpeg->ac[component->ac_table];
	int32_t length;
	uint32_t k = 1;

	length = huffman_decode(jpeg, dc);
	if (length < 0 || length > 16)
		return damaged_block(jpeg, error,
				     "a DC code its Huffman table does not hold");

	component->predictor += extend(take_bits(jpeg, (int)length),
				       (int)length);
	coefficients[0] = component->predictor * (int32_t)quant[0];

	while (k < 64) {
		int32_t rs = huffman_decode(jpeg, ac);
		int32_t run;
		int32_t size;

		if (rs < 0)
			return damaged_block(jpeg, error,
					     "an AC code its Huffman table does "
					     "not hold");

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
			return damaged_block(jpeg, error,
					     "a run of zeroes past the end of a "
					     "block");

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

// ------------------------------------------------------------------- the scan

// The scan header: which components are in it, and with which tables. Baseline
// has exactly one scan and it carries every component.
//
// A SCAN OF FEWER THAN ALL THE COMPONENTS IS LEGAL AND NOT READ: a baseline file
// may store each colour in a scan of its own, and saving it again joins them.
bool voe_assets_jpeg_read_scan(struct decoder *jpeg, const uint8_t *data,
			       uint32_t length, voe_base_error *error)
{
	uint32_t count;

	if (length < 1)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(JPEG: an empty SOS)");

	count = data[0];
	if (count >= 1 && count < jpeg->component_count)
		return voe_assets_jpeg_fail(
			error, VOE_BASE_ERROR_UNSUPPORTED,
			"the picture keeps its colours in separate scans, which "
			"the engine does not read; save it again as an RGB or "
			"greyscale JPEG, or as PNG (JPEG: a scan of %" PRIu32
			" of SOF0's %" PRIu32 " components)",
			count, jpeg->component_count);
	if (count != jpeg->component_count)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(JPEG: a scan of %" PRIu32
						    " components)",
					    count);
	if (length < 1 + count * 2 + 3)
		return voe_assets_jpeg_fail(error, VOE_BASE_ERROR_MALFORMED,
					    DAMAGED "(JPEG: SOS is shorter "
						    "than its %" PRIu32
						    " components)",
					    count);

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
			return voe_assets_jpeg_fail(
				error, VOE_BASE_ERROR_MALFORMED,
				DAMAGED "(JPEG: the scan names component %u, "
					"which SOF0 does not)",
				(unsigned)id);

		found->dc_table = tables >> 4;
		found->ac_table = tables & 15;
		if (found->dc_table >= 4 || found->ac_table >= 4)
			return voe_assets_jpeg_fail(
				error, VOE_BASE_ERROR_MALFORMED,
				DAMAGED "(JPEG: Huffman tables %u and %u, of "
					"the four there are)",
				(unsigned)found->dc_table,
				(unsigned)found->ac_table);
		if (!jpeg->dc[found->dc_table].present ||
		    !jpeg->ac[found->ac_table].present)
			return voe_assets_jpeg_fail(
				error, VOE_BASE_ERROR_MALFORMED,
				DAMAGED "(JPEG: component %u uses a Huffman "
					"table no DHT defines)",
				(unsigned)id);
	}

	return true;
}

// Every MCU, in order, each holding h*v blocks of each component.
bool voe_assets_jpeg_decode_scan(struct decoder *jpeg, voe_base_error *error)
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
							    my * component->v + by,
							    error))
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
