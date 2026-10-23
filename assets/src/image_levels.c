// A picture's levels (assets/image.h): the count of a full chain, the bytes of
// the levels a picture holds, the chain made level by level with the shrink in
// src/image_average.h, and the cooked form written, its key read from the head
// alone, and read back with no copy.
//
// THE HEAD IS SEVEN FIELDS AT FIXED OFFSETS, copied in and out with memcpy so
// nothing depends on how a struct would be padded or on `bytes` being aligned:
//   0 magic   4 version   8 key (8 bytes)   16 kind   20 width   24 height
//   28 levels, then level 0 at 32.
// Every field is native-endian (the header says why).
//
// THE READ TRUSTS NOTHING IN THE HEAD UNTIL IT IS BOUNDED. A side is held to
// VOE_ASSETS_IMAGE_MAX_SIDE and the levels to a full chain before any size is
// worked out, so the padded total cannot overflow and a damaged count cannot
// send `pixels` past the end of `bytes`.
#include "image_average.h"

#include <base/assert.h>

#include <string.h>

// "VOEP" in a little-endian machine's bytes; only equality matters.
#define COOKED_MAGIC 0x50454f56u
#define COOKED_VERSION 1u

#define AT_MAGIC 0
#define AT_VERSION 4
#define AT_KEY 8
#define AT_KIND 16
#define AT_WIDTH 20
#define AT_HEIGHT 24
#define AT_LEVELS 28

static_assert(VOE_ASSETS_IMAGE_COOKED_HEAD == AT_LEVELS + 4 &&
		      VOE_ASSETS_IMAGE_COOKED_HEAD % 16 == 0,
	      "the cooked head is its fields and keeps level 0 aligned");

static uint32_t read_u32(const uint8_t *at)
{
	uint32_t value;

	memcpy(&value, at, sizeof(value));
	return value;
}

static size_t padded(size_t size)
{
	return (size + 15) & ~(size_t)15;
}

// The bytes of `levels` levels of a width × height chain.
static size_t chain_size(uint32_t width, uint32_t height, uint32_t levels)
{
	size_t size = 0;

	for (uint32_t i = 0; i < levels; i++) {
		const uint32_t w = width >> i ? width >> i : 1;
		const uint32_t h = height >> i ? height >> i : 1;

		size += (size_t)w * h * 4;
	}
	return size;
}

uint32_t voe_assets_image_level_count(uint32_t width, uint32_t height)
{
	uint32_t longest = width > height ? width : height;
	uint32_t count = 1;

	VOE_BASE_ASSERT(width >= 1 && height >= 1,
			"the levels of a picture with no size");
	while (longest >>= 1)
		count++;
	return count;
}

size_t voe_assets_image_size(const voe_assets_image *image)
{
	return chain_size(image->width, image->height,
			  image->levels > 1 ? image->levels : 1);
}

voe_assets_image voe_assets_image_mips(voe_base_arena *arena,
				       const voe_assets_image *image,
				       voe_assets_image_kind kind)
{
	voe_assets_image out = *image;
	voe_assets_image above = *image;

	VOE_BASE_ASSERT(image->levels <= 1,
			"a picture already holding levels mipped again");
	VOE_BASE_ASSERT(image->pixels != NULL, "a picture with no pixels mipped");
	out.levels = voe_assets_image_level_count(image->width, image->height);
	out.pixels = voe_base_arena_push(
		arena, chain_size(out.width, out.height, out.levels));
	memcpy(out.pixels, image->pixels, (size_t)image->width * image->height * 4);

	above.pixels = out.pixels;
	for (uint32_t i = 1; i < out.levels; i++) {
		voe_assets_image level = {
			.width = above.width > 1 ? above.width / 2 : 1,
			.height = above.height > 1 ? above.height / 2 : 1,
			.pixels = above.pixels + (size_t)above.width * above.height * 4,
		};

		voe_assets_image_average(arena, &above, &level, kind);
		above = level;
	}
	return out;
}

voe_assets_bytes voe_assets_image_cooked_write(voe_base_arena *arena,
					       const voe_assets_image *image,
					       voe_assets_image_kind kind,
					       uint64_t key)
{
	// The key's two words are filled after, by memcpy at AT_KEY.
	const uint32_t head[8] = { COOKED_MAGIC,
				   COOKED_VERSION,
				   0,
				   0,
				   (uint32_t)kind,
				   image->width,
				   image->height,
				   image->levels > 1 ? image->levels : 1 };
	const size_t size = voe_assets_image_size(image);
	voe_assets_bytes out;

	VOE_BASE_ASSERT(image->pixels != NULL, "a picture with no pixels cooked");
	VOE_BASE_ASSERT(kind <= VOE_ASSETS_IMAGE_DATA, "a picture of no kind cooked");
	out.count = padded(VOE_ASSETS_IMAGE_COOKED_HEAD + size);
	out.bytes = voe_base_arena_push(arena, out.count);
	memcpy(out.bytes, head, sizeof(head));
	memcpy(out.bytes + AT_KEY, &key, sizeof(key));
	memcpy(out.bytes + VOE_ASSETS_IMAGE_COOKED_HEAD, image->pixels, size);
	memset(out.bytes + VOE_ASSETS_IMAGE_COOKED_HEAD + size, 0,
	       out.count - VOE_ASSETS_IMAGE_COOKED_HEAD - size);
	return out;
}

bool voe_assets_image_cooked_key(const uint8_t *head, size_t size,
				 uint64_t *key)
{
	if (size < VOE_ASSETS_IMAGE_COOKED_HEAD ||
	    read_u32(head + AT_MAGIC) != COOKED_MAGIC ||
	    read_u32(head + AT_VERSION) != COOKED_VERSION)
		return false;
	memcpy(key, head + AT_KEY, sizeof(*key));
	return true;
}

bool voe_assets_image_cooked_read(const uint8_t *bytes, size_t size,
				  voe_assets_image *image,
				  voe_assets_image_kind *kind, uint64_t *key,
				  size_t *used)
{
	uint32_t found_kind, width, height, levels;
	uint64_t found_key;
	size_t total;

	if (!voe_assets_image_cooked_key(bytes, size, &found_key))
		return false;
	found_kind = read_u32(bytes + AT_KIND);
	width = read_u32(bytes + AT_WIDTH);
	height = read_u32(bytes + AT_HEIGHT);
	levels = read_u32(bytes + AT_LEVELS);
	if (found_kind > VOE_ASSETS_IMAGE_DATA || width < 1 || height < 1 ||
	    width > VOE_ASSETS_IMAGE_MAX_SIDE ||
	    height > VOE_ASSETS_IMAGE_MAX_SIDE || levels < 1 ||
	    levels > voe_assets_image_level_count(width, height))
		return false;
	total = padded(VOE_ASSETS_IMAGE_COOKED_HEAD +
		       chain_size(width, height, levels));
	if (size < total)
		return false;

	// `pixels` is not const in the picture type; the caller only reads it,
	// so the const is dropped through an integer, not cast away silently.
	*kind = (voe_assets_image_kind)found_kind;
	*key = found_key;
	*image = (voe_assets_image){
		.width = width,
		.height = height,
		.pixels = (uint8_t *)(uintptr_t)(bytes +
						 VOE_ASSETS_IMAGE_COOKED_HEAD),
		.levels = levels,
	};
	*used = total;
	return true;
}
