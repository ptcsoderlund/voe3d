// The one call of app/include/app/picture.h: read, decode, upload, in that
// order, each failure handed straight back. The reasoning is in the header.
#include <app/picture.h>

#include <assets/image.h>
#include <base/assert.h>
#include <platform/file.h>

#include <stddef.h>

bool voe_app_picture_read(voe_render_device *device, const char *path,
			  voe_base_arena *scratch, voe_app_picture *out,
			  voe_base_error *error)
{
	voe_assets_image image = { 0 };
	voe_render_texture texture = { 0 };
	const uint8_t *bytes;
	size_t count = 0;

	VOE_BASE_ASSERT(device != NULL && path != NULL && scratch != NULL &&
				out != NULL,
			"a picture read needs a device, a path, scratch and out");

	bytes = voe_platform_file_read(path, scratch, &count, error);
	if (bytes == NULL)
		return false;
	if (!voe_assets_png_decode(bytes, count, scratch, &image, error))
		return false;
	if (!voe_render_texture_create(device, VOE_RENDER_TEXTURE_COLOUR,
				       VOE_RENDER_SAMPLING_SHARP, image.width,
				       image.height, image.pixels, &texture,
				       error))
		return false;

	*out = (voe_app_picture){
		.texture = texture,
		.width = image.width,
		.height = image.height,
	};
	VOE_BASE_ASSERT(out->width > 0 && out->height > 0,
			"a decoded picture has a size");
	return true;
}
