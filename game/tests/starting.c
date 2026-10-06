// The starting frame on a headless device opened as tests/interface.c opens
// one, at 640x360 so the line is some pixels tall. After one plain frame the
// window target read back holds the default theme's ground, sRGB-encoded, near
// a corner (inset past the panel's hairline border), and the middle row holds a
// pixel unlike it: the line. After the prepare loop the device answers
// PREPARED.
//
// The splash cases open a device of their own, much wider and then much taller
// than a 4x2 picture whose top-left texel is red and the rest blue: the margin
// beside (above) the picture is red, the centre blue, and the bottom middle
// 10 mm up the ground of the line's panel.
//
// The wait runs the shaders step on a fresh device's worker: the device ends
// PREPARED and the cache is written under the working directory, removed at
// the end; a work answering false makes the wait false. The progress line is
// checked for total 0 and for 3/7 with no device.
//
// A machine with no usable Vulkan skips and says so.
#include <game/frame.h>
#include <game/interface.h>
#include <game/progress.h>
#include <game/starting.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <platform/file.h>

#include <text/font.h>

#include <ui/theme.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDE 640
#define HIGH 360
#define LINE "Starting - preparing shaders..."
#define CACHE_ROOT "game_starting_test"
#define CACHE_FOLDER CACHE_ROOT "/voe3d"
#define CACHE_PATH CACHE_FOLDER "/pipelines_test.cache"

static const int red[3] = { 255, 0, 0 };
static const int blue[3] = { 0, 0, 255 };

// A linear channel as the window's sRGB format stores it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f
				? linear * 12.92f
				: 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

// How far the pixel at (x, y) is from `ground`, the largest channel's gap.
static int distance(const voe_render_picture *picture, uint32_t x, uint32_t y,
		    const int ground[3])
{
	const uint8_t *pixel = picture->pixels + (y * picture->width + x) * 4;
	int most = 0;

	for (int c = 0; c < 3; c++) {
		int gap = abs((int)pixel[c] - ground[c]);

		most = gap > most ? gap : most;
	}
	return most;
}

// The default theme's ground, derived with Oxanium as the interface derives
// it, as window bytes.
static void ground_bytes(voe_render_device *device, voe_base_arena *arena,
			 int ground[3])
{
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_text_font *font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM,
						device, arena, NULL);
	voe_ui_theme theme;

	VOE_TEST_CHECK(font != NULL);
	if (font == NULL)
		return;
	theme = voe_ui_theme_derive(&inputs, font);
	ground[0] = srgb_byte(theme.ground.x);
	ground[1] = srgb_byte(theme.ground.y);
	ground[2] = srgb_byte(theme.ground.z);
	voe_text_font_destroy(font);
}

static void starting_frame_draws_line(voe_app *app, voe_ui_context *ui,
				      voe_base_arena *arena,
				      voe_base_arena *scratch)
{
	int ground[3] = { -1000, -1000, -1000 };
	voe_render_picture picture;
	bool line_seen = false;

	ground_bytes(voe_app_device(app), arena, ground);
	VOE_TEST_CHECK(voe_game_starting_frame(app, ui, scratch, NULL, LINE));
	voe_base_arena_clear(scratch);
	VOE_TEST_CHECK(voe_render_target_read(voe_app_device(app),
					      VOE_RENDER_TARGET_WINDOW, scratch,
					      &picture, NULL));
	VOE_TEST_CHECK_INT(picture.width, WIDE);
	VOE_TEST_CHECK_INT(picture.height, HIGH);
	if (picture.width != WIDE || picture.height != HIGH)
		return;
	VOE_TEST_CHECK(distance(&picture, 16, 16, ground) <= 3);
	for (uint32_t x = 0; x < WIDE && !line_seen; x++)
		line_seen = distance(&picture, x, HIGH / 2, ground) > 40;
	VOE_TEST_CHECK(line_seen);
	voe_base_arena_clear(scratch);
}

static void starting_prepare_prepares(voe_app *app, voe_ui_context *ui,
				      voe_base_arena *scratch)
{
	VOE_TEST_CHECK(voe_game_starting_prepare(app, ui, scratch, NULL, LINE));
	VOE_TEST_CHECK(voe_render_device_prepare(voe_app_device(app)) ==
		       VOE_RENDER_PREPARED);
	voe_base_arena_clear(scratch);
}

// A 4x2 picture, its top-left texel red and the rest blue. False when the
// upload was refused.
static bool splash_make(voe_render_device *device, voe_app_picture *splash)
{
	uint8_t rgba[4 * 2 * 4];

	for (int t = 0; t < 8; t++) {
		const int *colour = t == 0 ? red : blue;

		for (int c = 0; c < 3; c++)
			rgba[t * 4 + c] = (uint8_t)colour[c];
		rgba[t * 4 + 3] = 255;
	}
	splash->width = 4;
	splash->height = 2;
	return voe_render_texture_create(device, VOE_RENDER_TEXTURE_COLOUR,
					 VOE_RENDER_SAMPLING_SHARP, 4, 2, rgba,
					 &splash->texture, NULL);
}

// One splash frame read back: red at (`margin_x`, `margin_y`), blue in the
// middle, the ground 10 mm above the bottom middle.
static void splash_check(voe_app *app, voe_ui_context *ui,
			 voe_base_arena *arena, voe_base_arena *scratch,
			 uint32_t margin_x, uint32_t margin_y)
{
	voe_render_device *device = voe_app_device(app);
	int ground[3] = { -1000, -1000, -1000 };
	voe_app_picture splash;
	voe_render_picture picture;
	voe_platform_size size;
	float millimetre;

	ground_bytes(device, arena, ground);
	VOE_TEST_CHECK(splash_make(device, &splash));
	VOE_TEST_CHECK(voe_game_starting_frame(app, ui, scratch, &splash, LINE));
	voe_base_arena_clear(scratch);
	if (!voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW, scratch,
				    &picture, NULL)) {
		VOE_TEST_CHECK(false);
		return;
	}
	size = (voe_platform_size){ (int)picture.width, (int)picture.height };
	millimetre = (float)picture.height / voe_game_interface_surface(size).y;
	VOE_TEST_CHECK(distance(&picture, margin_x, margin_y, red) <= 3);
	VOE_TEST_CHECK(distance(&picture, picture.width / 2, picture.height / 2,
				blue) <= 3);
	VOE_TEST_CHECK(distance(&picture, picture.width / 2,
				picture.height -
					(uint32_t)lroundf(10.0f * millimetre),
				ground) <= 3);
	(void)voe_render_texture_destroy(device, splash.texture);
	voe_base_arena_clear(scratch);
}

static void splash_fits_a_wide_window(voe_app *app, voe_ui_context *ui,
				      voe_base_arena *arena,
				      voe_base_arena *scratch)
{
	splash_check(app, ui, arena, scratch, 100, 160);
}

static void splash_fits_a_tall_window(voe_app *app, voe_ui_context *ui,
				      voe_base_arena *arena,
				      voe_base_arena *scratch)
{
	splash_check(app, ui, arena, scratch, 160, 100);
}

// The shaders step's context, as a program's work would hold it.
struct shaders_work {
	voe_render_device *device;
	const char *cache_path;
	voe_base_arena *scratch;
};

static bool shaders_run(void *context, voe_game_progress *progress)
{
	struct shaders_work *work = context;

	return voe_game_starting_shaders(work->device, work->cache_path,
					 work->scratch, progress);
}

static bool refusing_run(void *context, voe_game_progress *progress)
{
	(void)context;
	voe_game_progress_set(progress, "Refusing", 1, 1);
	return false;
}

// On a fresh device: the wait around the shaders step leaves it PREPARED and
// the cache written; a work answering false makes the wait false.
static void wait_cases(voe_app *app, voe_ui_context *ui, voe_base_arena *arena,
		       voe_base_arena *scratch)
{
	voe_base_arena *worker_scratch = voe_base_arena_new(1 << 20);
	struct shaders_work work = { voe_app_device(app), CACHE_PATH,
				     worker_scratch };

	(void)arena;
	VOE_TEST_CHECK(voe_game_starting_wait(app, ui, scratch, NULL,
					      shaders_run, &work));
	VOE_TEST_CHECK(voe_render_device_prepare(voe_app_device(app)) ==
		       VOE_RENDER_PREPARED);
	VOE_TEST_CHECK(voe_platform_file_exists(CACHE_PATH));
	VOE_TEST_CHECK(!voe_game_starting_wait(app, ui, scratch, NULL,
					       refusing_run, NULL));
	voe_base_arena_destroy(worker_scratch);
	voe_base_arena_clear(scratch);
}

static void progress_lines(void)
{
	voe_game_progress progress = { 0 };
	char line[64];

	voe_game_progress_set(&progress, "Starting", 0, 0);
	voe_game_progress_line(&progress, line, sizeof(line));
	VOE_TEST_CHECK(strcmp(line, "Starting") == 0);
	voe_game_progress_set(&progress, "Preparing shaders", 3, 7);
	voe_game_progress_line(&progress, line, sizeof(line));
	VOE_TEST_CHECK(strcmp(line, "Preparing shaders 3/7") == 0);
	VOE_TEST_CHECK(!voe_game_progress_stopped(&progress));
	VOE_TEST_CHECK(!voe_game_progress_stopped(NULL));
	voe_game_progress_set(NULL, "Nothing", 1, 2);
}

static void plain_cases(voe_app *app, voe_ui_context *ui,
			voe_base_arena *arena, voe_base_arena *scratch)
{
	starting_frame_draws_line(app, ui, arena, scratch);
	starting_prepare_prepares(app, ui, scratch);
}

// `cases` run on a headless device of `width` by `height` with an interface.
// False when there is no usable Vulkan, said once.
static bool on_device(int width, int height,
		      void (*cases)(voe_app *, voe_ui_context *,
				    voe_base_arena *, voe_base_arena *))
{
	voe_base_arena *arena = voe_base_arena_new(1 << 23);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = { .width = width,
				      .height = height,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_game_interface *interface;
	voe_app *app;

	app = voe_app_new_headless(arena, scratch, settings, &error);
	if (app == NULL) {
		bool skip = error == VOE_BASE_ERROR_UNAVAILABLE ||
			    error == VOE_BASE_ERROR_UNSUPPORTED;

		if (skip)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(app != NULL);
		voe_base_arena_destroy(scratch);
		voe_base_arena_destroy(arena);
		return false;
	}
	voe_base_arena_clear(scratch);

	interface = voe_game_interface_new(voe_app_device(app), arena);
	VOE_TEST_CHECK(interface != NULL);
	if (interface != NULL) {
		cases(app, voe_game_interface_context(interface), arena,
		      scratch);
		voe_game_interface_destroy(interface);
	}

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return true;
}

int main(void)
{
	progress_lines();
	if (on_device(WIDE, HIGH, plain_cases)) {
		(void)on_device(1280, 320, splash_fits_a_wide_window);
		(void)on_device(320, 640, splash_fits_a_tall_window);
		(void)on_device(WIDE, HIGH, wait_cases);
	}
	remove(CACHE_PATH);
	remove(CACHE_FOLDER);
	remove(CACHE_ROOT);
	return voe_test_result();
}
