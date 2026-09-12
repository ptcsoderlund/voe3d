// The startup pair, the frame's three readings, and the two calls that open and
// close a draw. Every line of reasoning is in app/include/app/app.h; what is
// here is the order, which is the part that is easy to get wrong.
#include <app/app.h>

#include <base/assert.h>
#include <platform/clock.h>

#include <stdio.h>

struct voe_app {
	voe_platform_window *window;
	voe_render_device *device;
	voe_app_clock clock;
	// Settings' ceiling, kept because every tick needs it and a program that
	// had to pass it back every frame could pass a different one by mistake.
	double longest_step;
};

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

voe_app *voe_app_new(voe_base_arena *arena, voe_base_arena *scratch,
		     voe_app_settings settings, voe_base_error *error)
{
	voe_platform_window *window;
	voe_render_device *device;
	voe_app *app;

	VOE_BASE_ASSERT(arena != NULL, "the struct needs an arena to live in");
	VOE_BASE_ASSERT(scratch != NULL, "startup needs scratch memory");
	VOE_BASE_ASSERT(settings.title != NULL, "a window needs a title");
	// Checked at startup rather than at the first tick, where the clock
	// asserts on the same thing: the same bug, found before a window opens.
	VOE_BASE_ASSERT(settings.longest_step > 0.0,
			"longest_step is a ceiling and has to be above zero");

	window = voe_platform_window_new(settings.width, settings.height,
					 settings.title);
	if (window == NULL) {
		fprintf(stderr, "app: the window would not open\n");
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

	// Last, so a startup that failed has put nothing in the caller's arena.
	app = voe_base_arena_push(arena, sizeof *app);
	*app = (voe_app){ .window = window,
			  .device = device,
			  .longest_step = settings.longest_step };

	report(error, VOE_BASE_OK);
	return app;
}

void voe_app_destroy(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "nothing to destroy");

	// The device holds a surface onto the window, so it goes first.
	voe_render_device_destroy(app->device);
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

voe_app_frame voe_app_frame_open(voe_app *app)
{
	voe_app_frame frame = { 0 };

	VOE_BASE_ASSERT(app != NULL, "no app to open a frame on");

	// The reading is taken before the poll so that the interval covers the
	// whole of the previous frame, the poll included.
	frame.tick = voe_app_clock_tick(&app->clock, voe_platform_clock_now(),
					app->longest_step);

	voe_platform_window_poll(app->window);
	frame.size = voe_platform_window_size(app->window);
	// A window with no area has nothing to draw into. Either dimension is
	// enough; both are zero when a window is minimised on either platform.
	frame.minimised = frame.size.width <= 0 || frame.size.height <= 0;
	frame.closing = voe_platform_window_should_close(app->window);
	return frame;
}

bool voe_app_draw_open(voe_app *app, voe_platform_size size,
		       voe_render_view view, voe_render_light light,
		       bool *drawing)
{
	VOE_BASE_ASSERT(app != NULL, "no app to draw with");
	VOE_BASE_ASSERT(drawing != NULL, "the caller has to be told whether to draw");

	if (!voe_render_frame_begin(app->device, size, view, light, drawing)) {
		fprintf(stderr, "app: the GPU stopped answering\n");
		return false;
	}
	return true;
}

bool voe_app_draw_close(voe_app *app)
{
	VOE_BASE_ASSERT(app != NULL, "no app to draw with");

	if (!voe_render_frame_end(app->device)) {
		fprintf(stderr, "app: the GPU stopped answering\n");
		return false;
	}
	return true;
}
