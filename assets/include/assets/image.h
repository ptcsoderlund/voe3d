// Images: bytes in, pixels out, and — for PNG — pixels in, bytes out. PNG and
// JPEG, both written here (ADR-0023), and nothing fetched.
//
// NOTHING HERE OPENS A FILE, IN EITHER DIRECTION, AND THAT IS A DECISION AND NOT
// A GAP. platform owns files and now has a call that writes one, so this could
// have taken a path; ADR-0157 says why it does not. Opening a file here would be
// assets doing the operating system's job, it would make the encoder untestable
// without a disk — its test decodes its own output instead — and it would
// give this folder a second way to fail that has nothing to do with a format. So
// the format work is what assets does, the bytes arrive from and go back to
// wherever the caller keeps them, and dev gets its image the way render gets its
// shaders: #embed at build time, which also keeps "nothing is read from disk at
// run time" true.
//
// THE OUTPUT IS ALWAYS RGBA8 AND NEVER THE FILE'S OWN LAYOUT. A greyscale PNG, a
// palette PNG and a JPEG all arrive here as four bytes a pixel with an opaque
// alpha where the file had none. One layout means one upload path in render, one
// image format on the GPU, and no branch anywhere downstream on what the file
// happened to be — which is worth more than the memory a greyscale image saves,
// especially as the GPU would pad it back out again.
//
// THERE IS NO VERTICAL FLIP IN THIS ENGINE, AND THAT IS THE DECISION THE CARD
// ASKED FOR RATHER THAN AN OVERSIGHT. Row zero of a PNG is the top row, Vulkan's
// texture coordinate (0,0) is the top-left texel, and glTF's UVs put (0,0) at
// the top-left of the image. All three already agree, so the rows are uploaded
// in the order they are decoded and the picture comes out the right way up. It
// binds both directions: the encoder writes the rows in the order it is handed
// them, so a picture saved and read back is the picture that went in.
//
// The flip everyone reaches for is OpenGL's: its texture origin is bottom-left,
// so every OpenGL loader turns the image over on the way in, and every tutorial
// written against one says to. Copying that here is how the picture ends up
// upside down — and then how it ends up flipped a second time by someone fixing
// it at the other end. If a texture is ever upside down, the bug is that
// somebody added a flip, not that this one is missing.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// The largest picture this engine will decode, per side.
//
// IT EXISTS SO THAT A HEADER CANNOT ASK FOR A TERABYTE. Width and height are
// four bytes each in a PNG and a decoder that believes them multiplies two
// numbers near four billion together to size an allocation. 16384 is above any
// texture this engine will draw and below the point where width * height * 4
// can overflow anything, so the multiply below is safe by construction rather
// than by a checked-arithmetic dance. A file declaring more is refused as
// malformed, which is the honest answer: it is not a picture, it is an ask.
#define VOE_ASSETS_IMAGE_MAX_SIDE 16384

// A decoded picture. `pixels` is width * height * 4 bytes of RGBA, row zero
// first, and it lives in the arena that was passed to the decoder — so it is
// freed by rewinding or destroying that arena and never one allocation at a
// time (rule 11).
typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *pixels;
} voe_assets_image;

// Decode a PNG. False on failure, and `image` is untouched when it fails.
//
// WHAT IS REFUSED, AND EACH OF THESE IS SAID OUT LOUD RATHER THAN GUESSED AT:
// interlaced PNG (VOE_BASE_ERROR_UNSUPPORTED — the file is fine, this decoder
// is not interested), and any bit depth but 8 (likewise). Everything else that
// goes wrong is VOE_BASE_ERROR_MALFORMED: a bad signature, a chunk whose CRC
// does not match, a truncated file, a declared size past
// VOE_ASSETS_IMAGE_MAX_SIDE, a compressed stream that does not produce exactly
// the number of bytes the header implies.
//
// Colour types 0, 2, 3, 4 and 6 at depth 8 are all read — greyscale, RGB,
// palette, greyscale with alpha, and RGBA — and all come out as RGBA8.
[[nodiscard]] bool voe_assets_png_decode(const uint8_t *bytes, size_t size,
					 voe_base_arena *arena,
					 voe_assets_image *image,
					 voe_base_error *error);

// A block of bytes in the arena that produced them: an encoded file, ready to be
// handed to whoever writes it out. `count` is how many of them there are, and
// the buffer is freed by rewinding or destroying that arena (rule 11).
typedef struct {
	uint8_t *bytes;
	size_t count;
} voe_assets_bytes;

// Encode a picture as a PNG. False on failure, and `out` is untouched when it
// fails.
//
// `image.pixels` is RGBA8, width * height * 4 bytes, row zero the top — exactly
// the shape _decode hands back, so a picture may be decoded, edited and written
// again with nothing in between.
//
// WHAT IS WRITTEN IS ONE SHAPE AND ONLY ONE, ON PURPOSE: colour type 6 (RGBA),
// eight bits a channel, no interlace, and filter 0 on every row. There is no
// argument to pick another, and rule 10 says why — nothing has asked. A reader
// that wants to know what else the format allows is holding the decoder beside
// this, which reads five colour types and all five filters.
//
// THE BYTES ARE THE CALLER'S ARENA'S, and so is the working memory this needs on
// the way — the filtered rows and the compressed stream. Nothing is freed one
// allocation at a time; the caller rewinds or destroys the arena once it has
// done whatever it was going to do with `out`.
//
// THE ONE RETURNED FAILURE IS A PICTURE TOO LARGE TO MEASURE: a width and height
// whose byte count does not fit a size_t is VOE_BASE_ERROR_REFUSED, because the
// machine cannot hold it rather than because the picture is wrong. A width or
// height of zero, and a NULL `pixels`, are the caller having lost track of its
// own data and assert (rule 13). Note there is no maximum side here — the ceiling
// VOE_ASSETS_IMAGE_MAX_SIDE puts on a decoder is a defence against a hostile
// file, and there is no file yet on this side.
//
// COLOUR IS WRITTEN EXACTLY AS IT IS HANDED OVER. No gamma is applied, nothing is
// premultiplied or un-premultiplied, and no channel is swapped. Whoever produced
// the pixels did those; ADR-0156 says who, for the pixels that come off a render
// target.
[[nodiscard]] bool voe_assets_png_encode(voe_base_arena *arena,
					 voe_assets_image image,
					 voe_assets_bytes *out,
					 voe_base_error *error);

// Decode a baseline JPEG. False on failure, and `image` is untouched when it
// fails.
//
// WHAT IS REFUSED: progressive JPEG (VOE_BASE_ERROR_UNSUPPORTED — it is a
// different decoder, not a broken file, and producing a corrupt picture from one
// would be worse than saying no), arithmetic coding, twelve-bit samples, and
// anything that is not one or three components — which means CMYK. Everything
// else that goes wrong is VOE_BASE_ERROR_MALFORMED.
//
// Greyscale and YCbCr are both read, at any chroma subsampling the format
// allows, and both come out as RGBA8 with an opaque alpha. JPEG has no
// transparency.
[[nodiscard]] bool voe_assets_jpeg_decode(const uint8_t *bytes, size_t size,
					  voe_base_arena *arena,
					  voe_assets_image *image,
					  voe_base_error *error);
