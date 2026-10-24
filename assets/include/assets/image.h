// Images: bytes in, pixels out, and — for PNG — pixels in, bytes out. PNG and
// JPEG, both written here (ADR-0023), and nothing fetched. A picture's mip
// chain, its BC7 or BC5 blocks (0477) and its cooked form, the copy kept in
// the cache (0470), are here too.
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
// than by a checked-arithmetic dance. A file declaring more is refused as a
// limit the engine keeps (ADR-0443), VOE_BASE_ERROR_UNSUPPORTED and not
// malformed: the file may be fine, and the artist is told to scale it down.
#define VOE_ASSETS_IMAGE_MAX_SIDE 16384

// How a picture's `pixels` are laid out. Zero is RGBA8, what every decoder
// hands back and what a zeroed picture means.
typedef enum {
	VOE_ASSETS_IMAGE_RGBA8 = 0,
	VOE_ASSETS_IMAGE_BC7,
	VOE_ASSETS_IMAGE_BC5
} voe_assets_image_format;

// A decoded picture. `pixels` is width * height * 4 bytes of RGBA, row zero
// first, and it lives in the arena that was passed to the decoder — so it is
// freed by rewinding or destroying that arena and never one allocation at a
// time (rule 11).
//
// `levels` of 0 or 1 means `pixels` is level 0 alone, which is what every
// decoder hands back and what a zeroed picture means. n means n levels one
// after another, level i being max(1, width >> i) × max(1, height >> i) RGBA8,
// the way Vulkan sizes a mip chain (0470 point 3).
//
// A BC7 or BC5 picture's `pixels` are 16-byte blocks of 4×4 texels instead:
// each level's ⌈w/4⌉ × ⌈h/4⌉, row by row, the levels back to back.
typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *pixels;
	uint32_t levels;
	voe_assets_image_format format;
} voe_assets_image;

// Decode a PNG. False on failure, and `image` is untouched when it fails.
//
// WHAT IS REFUSED, AND EACH OF THESE IS SAID OUT LOUD RATHER THAN GUESSED AT:
// interlaced PNG (VOE_BASE_ERROR_UNSUPPORTED — the file is fine, this decoder
// is not interested), bit depths below 8 and a palette at 16 (likewise), and a
// side past VOE_ASSETS_IMAGE_MAX_SIDE (likewise, a limit the engine keeps).
// Everything else that goes wrong is VOE_BASE_ERROR_MALFORMED: a bad
// signature, a chunk whose CRC does not match, a truncated file, a compressed
// stream that does not produce exactly the number of bytes the header implies.
// Every refusal prints one line in the artist's words (ADR-0445): what is
// wrong with the numbers, what to do, then `(PNG: …)`; the picture is the
// report's subject, pushed by whoever has it.
//
// Colour types 0, 2, 3, 4 and 6 at depth 8 are all read — greyscale, RGB,
// palette, greyscale with alpha, and RGBA — and all come out as RGBA8. Depth 16
// is read in the four that allow it, each sample rounded to 8 bits (0446).
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

// What a picture's texels mean, which decides how four of them become one
// (0466 point 3): a picture any material names as its normal map is NORMAL,
// else one named as a base colour or emission map is COLOUR, and every other
// one, the merged ORM included, is DATA.
typedef enum {
	VOE_ASSETS_IMAGE_COLOUR,
	VOE_ASSETS_IMAGE_NORMAL,
	VOE_ASSETS_IMAGE_DATA
} voe_assets_image_kind;

// A picture no larger than `largest` on its longest side.
//
// THE SIZE (0453, 0466 point 1). A picture whose longest side is at most
// `largest` is `image` itself, its pixels untouched and nothing pushed: nothing
// is ever enlarged. Otherwise the longest side becomes `largest` exactly and the
// other is scaled by the same factor, rounded, at least 1, and the result is a
// new picture in `arena`.
//
// AN AREA AVERAGE (0466 point 2): each written texel is the mean of the source
// texels under its footprint, an edge texel weighted by the part of it
// covered. It is the cheapest filter that is right at any ratio and keeps the
// picture's mean; a bilinear sample at a quarter skips three texels in four and
// shimmers.
//
// EACH KIND BY ITS USE. DATA averages each channel as stored. NORMAL decodes
// rgb to a vector (2c − 1), averages, makes it unit length again and encodes it
// back, so bumps neither flatten nor turn to noise; its alpha is averaged
// plainly. COLOUR averages rgb in linear light, sRGB decoded and encoded again,
// so contrast does not darken, and weighted by alpha, so a cut-out's hidden
// texels do not bleed into its edge (plain weights where all are transparent);
// alpha is averaged plainly.
//
// The working memory is one row of floats in `arena`, given back before the
// return, never a float copy of the picture. Row zero stays the top: no flip.
// `largest` of 0 and NULL pixels are the caller's bug and assert (rule 13).
voe_assets_image voe_assets_image_fit(voe_base_arena *arena,
				      voe_assets_image image, uint32_t largest,
				      voe_assets_image_kind kind);

// The levels of a full chain for a picture of that size, down to 1×1: one
// more than the number of times the longest side halves before it is 1.
// A side of zero is the caller's bug and asserts.
uint32_t voe_assets_image_level_count(uint32_t width, uint32_t height);

// The bytes of every level `image` holds, level 0 alone when `levels` is 0
// or 1: four a texel for RGBA8, sixteen a block for BC7 and BC5.
size_t voe_assets_image_size(const voe_assets_image *image);

// A copy of `image` with its full chain, in `arena` (0470 point 3).
//
// Level 0 is copied as it is. Each next level is made from the one before it
// by the area average of `kind`, exactly as voe_assets_image_fit shrinks, so
// a mip of a colour picture is in linear light and weighted by alpha and a
// mip of a normal map is unit length again.
//
// THE LEVELS SIT BACK TO BACK, LARGEST FIRST, so the small ones are one run
// of bytes at the end that can be read without the large ones, for texture
// streaming later (0454 point 4), and an upload is one copy per level with
// no gaps to skip.
//
// An input that already holds more than one level, or no pixels, is the
// caller's bug and asserts (rule 13).
voe_assets_image voe_assets_image_mips(voe_base_arena *arena,
				       const voe_assets_image *image,
				       voe_assets_image_kind kind);

// GPU-COMPRESSED PICTURES (0477 points 1–3): one byte a texel, a quarter of
// RGBA8, mips included. BC7 for colour and data, since it keeps alpha for a
// cut-out and has an sRGB form; BC5 for a normal map, since each of x and y
// gets eight levels of its own in a block and slopes stay smooth. A NORMAL
// picture keeps x and y alone, and whoever reads it makes z again.
//
// The source is RGBA8 with any number of levels, and the blocks follow its
// chain. A source already BC, or a run of blocks past the count, is the
// caller's bug and asserts (rule 13).
//
// The format a picture of `kind` is compressed to: BC5 for NORMAL, else BC7.
voe_assets_image_format voe_assets_image_blocks_format(voe_assets_image_kind kind);

// The blocks over every level `image` holds, whatever its format.
uint32_t voe_assets_image_block_count(const voe_assets_image *image);

// The compressed picture's shape and room in `arena`: `source`'s sides and
// levels, the format of `kind`, every block zeroed.
voe_assets_image voe_assets_image_blocks_new(voe_base_arena *arena,
					     const voe_assets_image *source,
					     voe_assets_image_kind kind);

// Blocks [first, first + count) of `target`, in chain order, encoded from
// `source`, so a caller can spread the work across frames (0477 point 3).
// `target` is what _blocks_new made for `source` and `kind`. An edge block's
// missing texels repeat the nearest edge texel.
void voe_assets_image_blocks_encode(const voe_assets_image *source,
				    voe_assets_image_kind kind,
				    voe_assets_image *target, uint32_t first,
				    uint32_t count);

// `source` compressed whole: _blocks_new, then every block encoded.
voe_assets_image voe_assets_image_compress(voe_base_arena *arena,
					   const voe_assets_image *source,
					   voe_assets_image_kind kind);

// THE COOKED FORM (0470 point 2): a head, then every level the picture holds,
// the total padded to 16 so a containing file can hold several in a row,
// each starting aligned. The head is magic, version, key, kind, width,
// height and levels, in that order.
//
// IT IS NATIVE-ENDIAN AND FOR THIS MACHINE'S CACHE ONLY. A cooked picture is a
// copy laid out as it sits in memory so a load is a read and a pointer (0454
// point 1); it is not an interchange format and is never read on another
// machine's byte order. The source picture is the file that travels.
//
// THE VERSION RISES WITH ANY CHANGE TO THE LAYOUT, or to how the levels are
// made. A copy whose magic or version is not this one's is stale and is made
// again from its source; nothing tries to read an older layout.
//
// The head's size in bytes, a multiple of 16.
#define VOE_ASSETS_IMAGE_COOKED_HEAD 32

// `image`, with every level it holds, as a cooked picture in `arena`.
// `key` is the caller's, kept in the head to tell a stale copy from a fresh
// one (0470: the source's stamp). Cannot fail; no pixels is the caller's bug
// and asserts.
voe_assets_bytes voe_assets_image_cooked_write(voe_base_arena *arena,
					       const voe_assets_image *image,
					       voe_assets_image_kind kind,
					       uint64_t key);

// The key in a cooked picture's head, read from the head alone, so a caller
// can tell a stale copy from its first bytes. False, `key` untouched, when
// `size` is short of a head or its magic or version is not this one's.
[[nodiscard]] bool voe_assets_image_cooked_key(const uint8_t *head, size_t size,
					       uint64_t *key);

// A cooked picture read with no copy: `image->pixels` points into `bytes`,
// which must outlive it and is read through it, never written. `*used` is
// the padded size, so the next picture
// in a containing file starts at `bytes + *used`.
//
// False, and every out-parameter untouched, when the magic or version is not
// this one's, the kind is unknown, a side is zero or past
// VOE_ASSETS_IMAGE_MAX_SIDE, the levels are more than a full chain, or `size`
// is short of the padded total. No line is printed: a stale or broken copy is
// made again from its source, not reported.
[[nodiscard]] bool voe_assets_image_cooked_read(const uint8_t *bytes,
						size_t size,
						voe_assets_image *image,
						voe_assets_image_kind *kind,
						uint64_t *key, size_t *used);

// Decode a baseline JPEG. False on failure, and `image` is untouched when it
// fails.
//
// WHAT IS REFUSED, EACH IN ONE LINE TO THE ARTIST AND THEN `(JPEG: …)`
// (ADR-0445); the picture is the report's subject, pushed by whoever has it:
// - progressive, arithmetic-coded, lossless or 12-bit: UNSUPPORTED, a
//   different decoder rather than a broken file; save it as a baseline JPEG
//   (untick Progressive) or as PNG.
// - a side past VOE_ASSETS_IMAGE_MAX_SIDE: UNSUPPORTED, a limit the engine
//   keeps; scale it down.
// - CMYK, or colours in separate scans: UNSUPPORTED; save it again as an RGB
//   or greyscale JPEG, or as PNG.
// - anything else is VOE_BASE_ERROR_MALFORMED: the picture is damaged, save
//   it again.
//
// Greyscale and YCbCr are both read, at any chroma subsampling the format
// allows, and both come out as RGBA8 with an opaque alpha. JPEG has no
// transparency.
[[nodiscard]] bool voe_assets_jpeg_decode(const uint8_t *bytes, size_t size,
					  voe_base_arena *arena,
					  voe_assets_image *image,
					  voe_base_error *error);
