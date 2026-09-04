// Images: bytes in, pixels out. PNG and JPEG, both written here (ADR-0023), and
// nothing fetched.
//
// A DECODER TAKES A BUFFER AND NOT A PATH, AND THAT IS THIS CARD READING ITS
// CARD NARROWLY RATHER THAN REACHING INTO platform. Card 017 says "a texture
// from a file", and platform — which owns files — has no file API yet;
// platform/window.h says so in as many words. Opening one here would be assets
// doing the operating system's job. So the format work is what assets does, the
// bytes arrive from wherever the caller got them, and dev gets its image the way
// render already gets its shaders: #embed at build time, which also keeps
// "nothing is read from disk at run time" true. The card that gives platform a
// file API is the card that makes this take a path, and it will not have to
// change anything below to do it.
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
// in the order they are decoded and the picture comes out the right way up.
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
