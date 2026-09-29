// The two startups, the frame's three readings, the two calls that open and
// close a draw, and the three steps of a capture. Every line of reasoning is in
// app/include/app/app.h; what is here is the order, which is the part that is
// easy to get wrong.
#include <app/app.h>

#include "pace.h"

#include <assets/image.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/clock.h>
#include <platform/file.h>

struct voe_app {
	// NULL on an app opened headless, and that is what every branch in this
	// file tests. There is no second flag: the window is either there or it
	// is not, and two ways of saying so could disagree.
	voe_platform_window *window;
	voe_render_device *device;
	voe_app_clock clock;
	// The size the settings named. Read only when there is no window, where
	// it is the whole of what a frame reports; a window is asked itself.
	voe_platform_size size;
	// Settings' ceiling, kept because every tick needs it and a program that
	// had to pass it back every frame could pass a different one by mistake.
	double longest_step;
	// The clock reading at which the previous frame opened: the pace's
	// heartbeat is measured from it.
	double last_open;
};

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// What both startups check, and a title is not one of them: a headless app has
// nothing to put one on.
static void check_settings(voe_base_arena *arena, voe_base_arena *scratch,
			   voe_app_settings settings)
{
	VOE_BASE_ASSERT(arena != NULL, "the struct needs an arena to live in");
	VOE_BASE_ASSERT(scratch != NULL, "startup needs scratch memory");
	// Checked at startup rather than at the first tick, where the clock
	// asserts on the same thing: the same bug, found before a window opens.
	VOE_BASE_ASSERT(settings.longest_step > 0.0,
			"longest_step is a ceiling and has to be above zero");
}

// The struct, written once for both startups so that a field added to it cannot
// be filled in one of them and forgotten in the other. Last in either path, so
// a startup that failed has put nothing in the caller's arena.
static voe_app *push_app(voe_base_arena *arena, voe_platform_window *window,
			 voe_render_device *device, voe_app_settings settings)
{
	voe_app *app = voe_base_arena_push(arena, sizeof *app);

	*app = (voe_app){ .window = window,
			  .device = device,
			  .size = { settings.width, settings.height },
			  .longest_step = settings.longest_step };
	return app;
}

voe_app *voe_app_new(voe_base_arena *arena, voe_base_arena *scratch,
		     voe_app_settings settings, voe_base_error *error)
{
	voe_platform_window *window;
	voe_render_device *device;
	voe_app *app;

	check_settings(arena, scratch, settings);
	VOE_BASE_ASSERT(settings.title != NULL, "a window needs a title");

	window = voe_platform_window_new(settings.width, settings.height,
					 settings.fullscreen, settings.title);
	if (window == NULL) {
		VOE_BASE_ERROR("app", "the window would not open");
		report(error, VOE_BASE_ERROR_UNAVAILABLE);
		return NULL;
	}

	// render keeps nothing out of scratch and says its own piece on stderr,
	// so there is nothing to add here beyond closing the window again.
	device = voe_render_device_new(scratch,
				       voe_platform_window_native(window),
				       voe_platform_window_size(window),
				       settings.capacities, error);
	if (device == NULL) {
		voe_platform_window_destroy(window);
		return NULL;
	}

	app = push_app(arena, window, device, settings);

	report(error, VOE_BASE_OK);
	return app;
}

voe_app *voe_app_new_headless(voe_base_arena *arena, voe_base_arena *scratch,
			      voe_app_settings settings, voe_base_error *error)
{
	voe_render_device *device;
	voe_app *app;
	voe_platform_size size = { settings.width, settings.height };

	check_settings(arena, scratch, settings);
	// With a window this is the window system's answer and may be anything;
	// here it is the whole of what gets drawn, so nought is the caller
	// having forgotten to say.
	VOE_BASE_ASSERT(size.width > 0 && size.height > 0,
			"a headless app draws at the size the settings name, so it has to be a size");

	// No window, so no native handle and nothing to close again on failure.
	// render says its own piece on stderr and keeps nothing out of scratch.
	device = voe_render_device_new_headless(scratch, size,
						settings.capacities, error);
	if (device == NULL)
		return NULL;

	app = push_app(arena, NULL, device, settings);

	report(error, VOE_BASE_OK);
	return app;
}

void voe_app_destroy(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "nothing to destroy");

	// The device holds a surface onto the window, so it goes first.
	voe_render_device_destroy(app->device);
	if (app->window != NULL)
		voe_platform_window_destroy(app->window);
}

voe_platform_window *voe_app_window(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "no app to ask");
	return app->window;
}

voe_render_device *voe_app_device(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "no app to ask");
	return app->device;
}

// The loop, not the wait, is what enforces the heartbeat: a wait may return at
// once, so each turn reads the window and the clock again and asks the step
// (ADR-0216). The poll after each wait is what makes those answers current, so
// regaining focus, being shown and a close request each end the wait early by
// turning the next step into a draw.
// The loop has no count on it by design: every wait it asks for is the rest of
// a quarter second that the clock is spending, and a hidden window is bounded
// by being shown or asked to close.
static void wait_for_pace(voe_app *app)
{
	voe_platform_window *window = app->window;
	voe_app_pace_step step;

	VOE_BASE_ASSERT(window != NULL, "a headless app is never paced");
	VOE_BASE_ASSERT(app->last_open >= 0.0, "a clock reading is not negative");

	for (;;) {
		step = voe_app_pace_next(voe_platform_window_focused(window),
					 voe_platform_window_visible(window),
					 voe_platform_window_should_close(window),
					 voe_platform_clock_now(),
					 app->last_open);
		if (step.kind == VOE_APP_PACE_DRAW)
			return;
		voe_platform_window_wait(window, step.seconds);
		voe_platform_window_poll(window);
	}
}

voe_app_frame voe_app_frame_open(voe_app *app)
{
	voe_app_frame frame = { 0 };

	VOE_BASE_ASSERT(app != NULL, "no app to open a frame on");

	if (app->window != NULL)
		wait_for_pace(app);

	// The reading is taken before the poll so that the interval covers the
	// whole of the previous frame, the poll included.
	frame.tick = voe_app_clock_tick(&app->clock, voe_platform_clock_now(),
					app->longest_step);
	app->last_open = frame.tick.now;

	// No window to poll and nothing to ask: the size is the one the settings
	// named, nothing minimises it and nothing will ever close it. The clock
	// above ticked first, so a headless loop advances a world exactly as a
	// windowed one does — which is what makes it the same loop.
	if (app->window == NULL) {
		frame.size = app->size;
		return frame;
	}

	voe_platform_window_poll(app->window);
	frame.size = voe_platform_window_size(app->window);
	// A window with no area has nothing to draw into. Either dimension is
	// enough; both are zero when a window is minimised on either platform.
	frame.minimised = frame.size.width <= 0 || frame.size.height <= 0;
	frame.closing = voe_platform_window_should_close(app->window);
	return frame;
}

bool voe_app_draw_open(voe_app *app, voe_platform_size size, bool *drawing)
{
	VOE_BASE_ASSERT(app != NULL, "no app to draw with");
	VOE_BASE_ASSERT(drawing != NULL, "the caller has to be told whether to draw");

	if (!voe_render_frame_begin(app->device, size, drawing)) {
		VOE_BASE_ERROR("app", "the GPU stopped answering");
		return false;
	}
	return true;
}

bool voe_app_draw_close(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "no app to draw with");

	if (!voe_render_frame_end(app->device)) {
		VOE_BASE_ERROR("app", "the GPU stopped answering");
		return false;
	}
	return true;
}

bool voe_app_capture_png(voe_app *app, voe_render_target target,
			 voe_base_arena *scratch, const char *path,
			 voe_base_error *error)
{
	voe_render_picture picture = { 0 };
	voe_assets_bytes file = { 0 };
	voe_assets_image image;

	VOE_BASE_ASSERT(app != NULL, "no app to capture from");
	VOE_BASE_ASSERT(scratch != NULL, "a capture needs an arena to work in");
	VOE_BASE_ASSERT(path != NULL, "a capture needs a path to write to");

	// The three steps, in the only order they go in, each one's failure
	// returned as it stands: the category is already the right one and the
	// line saying what actually happened is already on stderr. A step that
	// refused stops the next, so a picture that never came back is never
	// encoded and a file whose bytes do not exist is never opened.
	if (!voe_render_target_read(app->device, target, scratch, &picture,
				    error))
		return false;

	// The two structs are the same three fields, and the assignment is where
	// render's RGBA8 and assets' RGBA8 meet — see ADR-0156 for why neither
	// side converts anything here.
	image = (voe_assets_image){ .width = picture.width,
				    .height = picture.height,
				    .pixels = picture.pixels };
	if (!voe_assets_png_encode(scratch, image, &file, error))
		return false;

	return voe_platform_file_write(path, file.bytes, file.count, error);
}
