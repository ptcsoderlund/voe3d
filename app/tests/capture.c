// A program with no display draws a frame and writes it to a PNG file, and the
// file holds the picture that was drawn. Everything spec 003 asks of `app` in
// one run: a headless startup, the ordinary loop on it, and the three-step
// capture.
//
// THE PICTURE IS DELIBERATELY ASYMMETRICAL ON BOTH AXES, because a picture
// turned over or mirrored is the failure that looks perfectly plausible. Red
// left half, blue right half, and a small green mark in the top-left corner
// only: a mirrored picture swaps red and blue, and a picture upside down leaves
// the mark at the bottom. Neither survives a count.
//
// WHAT IS WRITTEN IS READ BACK THROUGH THE DECODER, NOT BY EYE. `app` writes a
// file and nothing in this engine opens one, so the test reads the bytes with
// fopen/fread — the same thing platform/tests/file.c does — and hands them to
// voe_assets_png_decode. The claim is then the whole chain: the card's pixels,
// render's channel swap, the encoder, the file, and the decoder beside it.
//
// ONE MILLIMETRE IS ONE PIXEL, on purpose: the element surface is handed the
// target's own size, so every rectangle's edges land exactly on pixel
// boundaries and every count below is exact rather than a threshold.
//
// A BAD PATH IS A RETURNED FAILURE AND NOT A CRASH, and nothing is written. The
// directory named does not exist, which is platform's VOE_BASE_ERROR_UNAVAILABLE
// — the picture and the encoding both succeeded, so this is the third step
// refusing and the one that proves a failure comes back out of the call rather
// than being swallowed.
//
// It writes one file into the working directory and removes it at the end, pass
// or fail, exactly as platform/tests/file.c does.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <app/app.h>

#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float4.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>

#define WIDTH 64
#define HEIGHT 48
#define HALF (WIDTH / 2)
#define MARK 8

// The file this writes, in the working directory ctest runs the test in.
#define CAPTURE_PATH "app_capture_test.png"
// A directory that is not there, so the write is refused before anything is
// created.
#define BAD_PATH "app_capture_no_such_directory/shot.png"

// Three rectangles and one pass; everything else is the smallest a device will
// open with, because nothing here draws a mesh.
static const voe_render_capacities CAPACITIES = {
	.vertices = 1,
	.indices = 1,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.elements = 3,
	.passes = 1,
};

static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const voe_math_float4 BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };

// One rectangle, clipped to itself — which is what an element that is not meant
// to be clipped says, because a zeroed clip rect clips everything away.
static voe_render_element solid(float x, float y, float w, float h,
				voe_math_float4 colour)
{
	return (voe_render_element){
		.bounds = { x, y, w, h },
		.clip = { x, y, w, h },
		.colour = colour,
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
}

#define NEITHER 0
#define IS_RED 1
#define IS_GREEN 2
#define IS_BLUE 3

// RGBA, which is what the decoder hands back and what render's readback put in
// the file. A channel swap anywhere in the chain turns every red count into a
// blue one.
static int colour_of(const uint8_t *pixel)
{
	unsigned red = pixel[0];
	unsigned green = pixel[1];
	unsigned blue = pixel[2];

	if (red > 128 && red > green && red > blue)
		return IS_RED;
	if (green > 128 && green > red && green > blue)
		return IS_GREEN;
	if (blue > 128 && blue > red && blue > green)
		return IS_BLUE;
	return NEITHER;
}

static const uint8_t *pixel_of(const voe_assets_image *image, uint32_t x,
			       uint32_t y)
{
	return image->pixels + ((size_t)y * image->width + x) * 4;
}

static int count_of(const voe_assets_image *image, int colour)
{
	int found = 0;

	for (uint32_t y = 0; y < image->height; y++) {
		for (uint32_t x = 0; x < image->width; x++) {
			if (colour_of(pixel_of(image, x, y)) == colour)
				found++;
		}
	}
	return found;
}

static int opaque_count(const voe_assets_image *image)
{
	int found = 0;

	for (uint32_t y = 0; y < image->height; y++) {
		for (uint32_t x = 0; x < image->width; x++) {
			if (pixel_of(image, x, y)[3] == 255)
				found++;
		}
	}
	return found;
}

// One frame through the ordinary loop — open, draw open, a pass, the three
// rectangles, close — and nothing about it is headless except that there is no
// window to have polled.
static void draw_one_frame(voe_app *app)
{
	voe_render_device *device = voe_app_device(app);
	voe_app_frame frame = voe_app_frame_open(app);
	voe_math_float2 surface = { WIDTH, HEIGHT };
	bool drawing = false;

	// What a headless frame reports, and it is the reason a headless loop
	// has to stop itself.
	VOE_TEST_CHECK(!frame.closing);
	VOE_TEST_CHECK(!frame.minimised);
	VOE_TEST_CHECK_INT(frame.size.width, WIDTH);
	VOE_TEST_CHECK_INT(frame.size.height, HEIGHT);

	VOE_TEST_CHECK(voe_app_draw_open(app, frame.size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;

	// No camera: nothing in this pass is a mesh, and the element transform
	// is the whole of what places a rectangle.
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));

	// Element space, millimetres, y down: (0,0) is the top-left corner. The
	// mark is submitted last, so paint order puts it over the red half.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HEIGHT, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, 0, HALF, HEIGHT, BLUE)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, MARK, MARK, GREEN)));

	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform(surface), 0,
		voe_render_frame_elements_submitted(device)));

	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_app_draw_close(app));
}

// The written file, back as bytes. NULL when it is not there, which is what the
// bad-path case checks for.
static uint8_t *read_whole_file(const char *path, voe_base_arena *arena,
				size_t *count)
{
	FILE *file = fopen(path, "rb");
	long length;
	uint8_t *bytes;

	*count = 0;
	if (file == NULL)
		return NULL;

	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0) {
		fclose(file);
		return NULL;
	}
	rewind(file);

	bytes = voe_base_arena_push(arena, (size_t)length);
	if (fread(bytes, 1, (size_t)length, file) != (size_t)length) {
		fclose(file);
		return NULL;
	}
	fclose(file);

	*count = (size_t)length;
	return bytes;
}

static void the_captured_file_is_the_picture_that_was_drawn(voe_app *app,
							    voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;
	voe_assets_image image = { 0 };
	uint8_t *file;
	size_t count;

	VOE_TEST_CHECK(voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW, arena,
					   CAPTURE_PATH, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);

	file = read_whole_file(CAPTURE_PATH, arena, &count);
	VOE_TEST_CHECK(file != NULL);
	if (file == NULL)
		return;

	VOE_TEST_CHECK(voe_assets_png_decode(file, count, arena, &image,
					     &error));
	if (image.pixels == NULL)
		return;

	VOE_TEST_CHECK_INT(image.width, WIDTH);
	VOE_TEST_CHECK_INT(image.height, HEIGHT);

	// ORIENTATION AND HANDEDNESS, AS THREE PIXELS. The mark is at the top of
	// the left half and the bottom of that half is plain red, so a picture
	// turned over fails the first two; the right half is blue, so a mirrored
	// one fails the third.
	VOE_TEST_CHECK_INT(colour_of(pixel_of(&image, 2, 2)), IS_GREEN);
	VOE_TEST_CHECK_INT(colour_of(pixel_of(&image, 2, HEIGHT - 3)), IS_RED);
	VOE_TEST_CHECK_INT(colour_of(pixel_of(&image, WIDTH - 3, 2)), IS_BLUE);

	// And the areas are exact, which is what says nothing leaked across the
	// halves and that the mark is the size it was asked for.
	VOE_TEST_CHECK_INT(count_of(&image, IS_GREEN), MARK * MARK);
	VOE_TEST_CHECK_INT(count_of(&image, IS_BLUE), HALF * HEIGHT);
	VOE_TEST_CHECK_INT(count_of(&image, IS_RED),
			   HALF * HEIGHT - MARK * MARK);
	VOE_TEST_CHECK_INT(count_of(&image, NEITHER), 0);

	// Everything drawn was opaque and comes back opaque: the un-premultiply
	// on the way off the card divided by one and left the picture alone.
	VOE_TEST_CHECK_INT(opaque_count(&image), WIDTH * HEIGHT);
}

static void a_path_that_cannot_be_opened_is_refused(voe_app *app,
						    voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;
	size_t count;

	VOE_TEST_CHECK(!voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW,
					    arena, BAD_PATH, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	// And nothing was created: the path is still not there to read.
	VOE_TEST_CHECK(read_whole_file(BAD_PATH, arena, &count) == NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = {
		.width = WIDTH,
		.height = HEIGHT,
		.capacities = CAPACITIES,
		.longest_step = 0.25,
	};
	voe_base_error error = VOE_BASE_OK;
	voe_app *app;

	// No title, which is the headless startup's one relaxation: there is
	// nothing to put one on.
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

	// An app with no window says so, rather than handing back something to
	// ask about a pointer that is not there.
	VOE_TEST_CHECK(voe_app_window(app) == NULL);
	VOE_TEST_CHECK(voe_app_device(app) != NULL);

	draw_one_frame(app);

	// Startup kept nothing out of scratch, so the captures work in it.
	voe_base_arena_destroy(scratch);
	scratch = voe_base_arena_new(1 << 20);

	the_captured_file_is_the_picture_that_was_drawn(app, scratch);
	a_path_that_cannot_be_opened_is_refused(app, scratch);

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);

	// Pass or fail, the working directory is left as it was found.
	remove(CAPTURE_PATH);
	return voe_test_result();
}
