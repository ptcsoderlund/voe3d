// What the two parts of the JPEG reader share, internal to this folder: the
// decoder's state, the Huffman table and component it holds, the refusal helper,
// and the calls jpeg.c makes into jpeg_scan.c. jpeg.c holds the markers, the
// colour conversion and the entry point; jpeg_scan.c holds the entropy-coded data.
//
// THE SPLIT IS BY STAGE, BECAUSE A JPEG IS SIX OF THEM AND EACH IS WRONG ON ITS
// OWN. A container of tables, then Huffman-coded coefficients, then a
// dequantise, then an inverse cosine transform, then a chroma upsample, then a
// colour conversion — each can produce a picture that merely looks odd rather
// than failing. The tables and the colours are read from the markers and are
// jpeg.c's; everything between the scan header and the next marker is
// jpeg_scan.c's, so a change to how a block decodes reads one file.
#pragma once

#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// Three is a colour JPEG and one is greyscale. Four is CMYK, which is a
// different colour conversion and has no caller.
#define MAX_COMPONENTS 3

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

// `code` into `error` when there is one; always false. In jpeg.c.
bool voe_assets_jpeg_fail(voe_base_error *error, voe_base_error code);

// The scan header: which components are in it, and with which tables. In
// jpeg_scan.c.
[[nodiscard]] bool voe_assets_jpeg_read_scan(struct decoder *jpeg,
					     const uint8_t *data,
					     uint32_t length);

// Every MCU, in order, into the component planes. In jpeg_scan.c.
[[nodiscard]] bool voe_assets_jpeg_decode_scan(struct decoder *jpeg);
