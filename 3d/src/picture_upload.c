// One picture into one SMOOTH texture by its shape (3d/src/picture_upload.h):
// blocks through _create_blocks, an RGBA8 chain through _create_levels, one
// RGBA8 level through _create with the GPU mipping it.
//
// Constraints: `assets`' format and `render`'s blocks are two enums because
// `assets` may not name `render`; the switch between them is total, so its
// fall-through is unreachable rather than a fallback. A BC picture's `levels`
// of 0 means one level, as it does for RGBA8.
#include "picture_upload.h"

#include <base/assert.h>

// `render`'s name for `format`'s blocks; RGBA8 is not blocks and asserts.
static voe_render_blocks blocks_of(voe_assets_image_format format)
{
	switch (format) {
	case VOE_ASSETS_IMAGE_BC7:
		return VOE_RENDER_BLOCKS_BC7;
	case VOE_ASSETS_IMAGE_BC5:
		return VOE_RENDER_BLOCKS_BC5;
	case VOE_ASSETS_IMAGE_RGBA8:
		break;
	}
	VOE_BASE_ASSERT(false, "an RGBA8 picture has no blocks");
	return VOE_RENDER_BLOCKS_BC7;
}

bool voe_3d_picture_upload(voe_render_device *device,
			   const voe_assets_image *image,
			   voe_render_texture_kind kind, voe_render_texture *out,
			   voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL && out != NULL,
			"uploading a picture needs a device and an id to fill");
	VOE_BASE_ASSERT(image != NULL && image->pixels != NULL,
			"uploading a picture of no pixels");
	VOE_BASE_ASSERT(image->width > 0 && image->height > 0,
			"uploading a picture of no size");

	const uint32_t levels = image->levels > 1 ? image->levels : 1;

	if (image->format != VOE_ASSETS_IMAGE_RGBA8)
		return voe_render_texture_create_blocks(
			device, kind, blocks_of(image->format), image->width,
			image->height, levels, image->pixels, out, error);
	if (levels > 1)
		return voe_render_texture_create_levels(device, kind,
							image->width,
							image->height, levels,
							image->pixels, out, error);
	return voe_render_texture_create(device, kind,
					 VOE_RENDER_SAMPLING_SMOOTH,
					 image->width, image->height,
					 image->pixels, out, error);
}
