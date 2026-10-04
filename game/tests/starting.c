// The starting frame on a headless device opened as tests/interface.c opens
// one, at 640x360 so the line is some pixels tall. After one frame the window
// target read back holds the default theme's ground, sRGB-encoded, near a
// corner (inset past the panel's hairline border), and the middle row holds a
// pixel unlike it: the line. After the prepare loop the device answers
// PREPARED.
//
// A machine with no usable Vulkan skips and says so.
#include <game/frame.h>
#include <game/interface.h>
#include <game/starting.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <text/font.h>

#include <ui/theme.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDE 640
#define HIGH 360
#define LINE "Starting - preparing shaders..."

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
	VOE_TEST_CHECK(voe_game_starting_frame(app, ui, scratch, LINE));
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
	VOE_TEST_CHECK(voe_game_starting_prepare(app, ui, scratch, LINE));
	VOE_TEST_CHECK(voe_render_device_prepare(voe_app_device(app)) ==
		       VOE_RENDER_PREPARED);
	voe_base_arena_clear(scratch);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 23);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = { .width = WIDE,
				      .height = HIGH,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_game_interface *interface;
	voe_app *app;

	app = voe_app_new_headless(arena, scratch, settings, &error);
	if (app == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(app != NULL);
		voe_base_arena_destroy(scratch);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	voe_base_arena_clear(scratch);

	interface = voe_game_interface_new(voe_app_device(app), arena);
	VOE_TEST_CHECK(interface != NULL);
	if (interface != NULL) {
		voe_ui_context *ui = voe_game_interface_context(interface);

		starting_frame_draws_line(app, ui, arena, scratch);
		starting_prepare_prepares(app, ui, scratch);
		voe_game_interface_destroy(interface);
	}

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
