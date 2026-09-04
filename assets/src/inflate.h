// DEFLATE (RFC 1951) inside a zlib wrapper (RFC 1950), which is the compression
// PNG uses and the only compression this engine has. Internal to assets: nothing
// outside decodes a stream, and a public "unzip these bytes" is surface with no
// caller (rule 10).
//
// THE OUTPUT SIZE IS AN INPUT, AND THAT IS THE WHOLE SECURITY DESIGN. A caller
// says how many bytes it expects and gets exactly that many or a failure. PNG
// knows its decompressed size before it decompresses — height rows of a width
// computed from the header — so there is nothing to discover and no buffer to
// grow. A decompressor that grows its own output is how a few hundred bytes of
// hostile file turns into a gigabyte of memory, and refusing to have that
// ability is cheaper than defending it.
#pragma once

#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// Decompress `size` bytes at `bytes` into exactly `capacity` bytes at `out`.
//
// Exactly, in both directions: a stream that ends early is truncated and a
// stream that keeps going is lying about its dimensions, and both are
// VOE_BASE_ERROR_MALFORMED. Trailing bytes after the stream ends are ignored,
// because PNG splits its stream across IDAT chunks and the last one may be
// padded by an encoder that had a block to round out.
[[nodiscard]] bool voe_assets_inflate(const uint8_t *bytes, size_t size,
				      uint8_t *out, size_t capacity,
				      voe_base_error *error);
