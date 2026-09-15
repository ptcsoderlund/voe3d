// The compressor: bytes in, a zlib stream out. Internal to assets, the mirror
// of inflate.h, and written here rather than fetched (rule 5, ADR-0023).
//
// IT PRODUCES A ZLIB STREAM AND NOT A BARE DEFLATE ONE — the two header bytes,
// the compressed data, then the Adler-32 of the input, big-endian. That is what
// inflate.h consumes and exactly what a PNG IDAT holds, so the encoder beside it
// hands these bytes to a chunk writer and adds nothing.
//
// IT CANNOT FAIL, WHICH IS WHY IT RETURNS NOTHING. There is one allocation, it
// comes from the caller's arena, and rule 11 makes a failed push fatal rather
// than a returned NULL. Nothing else in here can go wrong: every input is a
// valid input, the output size is bounded before a bit is written, and there is
// no format to be malformed because this side is the one writing it.
//
// THE OUTPUT BUFFER IS PUSHED AT THE WORST CASE AND THE USED LENGTH COMES BACK
// IN `out`. The worst case is fixed-Huffman literals: a literal costs at most
// nine bits (symbols 144-255), and no match is ever more expensive per byte than
// that — the dearest match is a length of three at the farthest distance, 25
// bits for three bytes, against 27 for three literals. So the data is at most
// `count * 9 / 8` bytes, which is `count + count / 8`, and 64 covers the two
// zlib header bytes, the three-bit block header, the seven-bit end-of-block
// symbol, the four Adler-32 bytes and the rounding in that division.
//
// A COUNT OF ZERO ASSERTS. An empty zlib stream is a well-defined thing, but
// nothing in this engine has one to compress: a PNG always has at least one
// filtered row. Compressing nothing is the caller having lost track of its own
// data (rule 13), and rule 10 says the empty case is written when something
// needs it.
#pragma once

#include <base/arena.h>

#include <stddef.h>
#include <stdint.h>

// The compressed stream, and how much of the buffer it used.
typedef struct {
	uint8_t *bytes;
	size_t count;
} voe_assets_deflate_result;

// Compress `count` bytes at `data` into a zlib stream pushed on `arena`.
void voe_assets_deflate(voe_base_arena *arena, const uint8_t *data,
			size_t count, voe_assets_deflate_result *out);
