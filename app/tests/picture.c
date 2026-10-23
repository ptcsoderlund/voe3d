// A PNG file comes back as a texture with its size, and a file that is not
// there or is not a PNG is refused (app/include/app/picture.h).
//
// THE PICTURE IS MADE HERE, NOT SHIPPED: a 4x2 RGBA picture is encoded with
// voe_assets_png_encode and written into the working directory, not the system
// temp folder, with voe_platform_file_write, so the test needs no file in the
// tree. 4 by 2 is not square, so a width and height swapped anywhere fails the
// check.
//
// Both files written are removed at the end, pass or fail.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO, as capture.c does.
#include <app/app.h>
#include <app/picture.h>

#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
#include <platform/file.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>

#define WIDTH 4
#define HEIGHT 2

static const voe_render_capacities CAPACITIES = {
	.vertices = 1,
	.indices = 1,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 1,
	.passes = 1,
};

static const char *const png_path = "voe_app_picture_test.png";
static const char *const text_path = "voe_app_picture_test.txt";

static void reads_a_png(voe_render_device *device, voe_base_arena *scratch)
{
	uint8_t pixels[WIDTH * HEIGHT * 4];
	voe_assets_image image = {
		.width = WIDTH,
		.height = HEIGHT,
		.pixels = pixels,
	};
	voe_assets_bytes encoded = { 0 };
	voe_app_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	for (size_t i = 0; i < sizeof(pixels); i++)
		pixels[i] = (uint8_t)(i * 7);
	VOE_TEST_CHECK(voe_assets_png_encode(scratch, image, &encoded, &error));
	VOE_TEST_CHECK(voe_platform_file_write(png_path, encoded.bytes,
					       encoded.count, &error));

	VOE_TEST_CHECK(voe_app_picture_read(device, png_path, scratch, &picture,
					    &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	VOE_TEST_CHECK_INT(picture.width, WIDTH);
	VOE_TEST_CHECK_INT(picture.height, HEIGHT);
	VOE_TEST_CHECK(picture.texture.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, picture.texture));
}

static void missing_file_fails(voe_render_device *device,
			       voe_base_arena *scratch)
{
	voe_app_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_app_picture_read(device,
					     "voe_app_picture_no_such_file.png",
					     scratch, &picture, &error));
	VOE_TEST_CHECK(error != VOE_BASE_OK);
	VOE_TEST_CHECK_INT(picture.width, 0);
}

static void not_a_png_fails(voe_render_device *device,
			    voe_base_arena *scratch)
{
	static const char TEXT[] = "this is text, not a picture\n";
	voe_app_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_platform_file_write(text_path, (const uint8_t *)TEXT,
					       sizeof(TEXT) - 1, &error));
	VOE_TEST_CHECK(!voe_app_picture_read(device, text_path, scratch,
					     &picture, &error));
	VOE_TEST_CHECK(error != VOE_BASE_OK);
	VOE_TEST_CHECK_INT(picture.width, 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = {
		.width = 16,
		.height = 16,
		.capacities = CAPACITIES,
		.longest_step = 0.25,
	};
	voe_base_error error = VOE_BASE_OK;
	voe_app *app;

	app = voe_app_new_headless(arena, scratch, settings, &error);
	if (app == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
		} else {
			VOE_TEST_CHECK(app != NULL);
		}
		voe_base_arena_destroy(scratch);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	reads_a_png(voe_app_device(app), scratch);
	missing_file_fails(voe_app_device(app), scratch);
	not_a_png_fails(voe_app_device(app), scratch);

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	remove(png_path);
	remove(text_path);
	return voe_test_result();
}
