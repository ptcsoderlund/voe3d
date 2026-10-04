// The starting frame and the prepare loop, as game/include/game/starting.h
// gives. The element records reach the pass as frame.c's interface draw sends
// them, less the depth clear: a pass with no camera has no depth to clear.
#include <game/starting.h>

#include <game/interface.h>

#include <base/assert.h>

#include <platform/window.h>

#include <render/device.h>

#include <ui/widgets.h>

#include <stdint.h>

// The window polled and its size, or the headless app's frame opened for the
// settings' size. False when the window is closing.
static bool starting_size(voe_app *app, voe_platform_size *size)
{
	voe_platform_window *window = voe_app_window(app);

	if (window == NULL) {
		*size = voe_app_frame_open(app).size;
		return true;
	}
	voe_platform_window_poll(window);
	if (voe_platform_window_should_close(window))
		return false;
	*size = voe_platform_window_size(window);
	return true;
}

// `line` centred on a GROUND panel as large as `surface`. False when the
// frame was refused.
static bool starting_layout(voe_ui_context *ui, voe_base_arena *frame_arena,
			    voe_math_float2 surface, const char *line)
{
	voe_ui_frame_begin(ui, frame_arena);
	voe_ui_panel_begin(
		ui, "starting", 0, VOE_UI_SURFACE_GROUND,
		(voe_ui_container){
			.size = { .along = { VOE_UI_SIZE_FIXED, surface.y },
				  .across = { VOE_UI_SIZE_FIXED, surface.x } },
			.along = VOE_UI_ALONG_CENTER,
			.across = VOE_UI_ACROSS_CENTER });
	voe_ui_label(ui, line);
	voe_ui_end(ui);
	return voe_ui_frame_end(ui);
}

// The context's records in one element draw, in one NULL-camera window pass.
static bool starting_draw(voe_render_device *device, const voe_ui_context *ui,
			  voe_platform_size size)
{
	uint32_t records = voe_ui_element_count(ui);
	uint32_t first;
	bool ok = true;

	if (!voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW, NULL))
		return false;
	first = voe_render_frame_elements_submitted(device);
	for (uint32_t e = 0; e < records && ok; e++)
		ok = voe_render_frame_submit_element(device,
						     voe_ui_element(ui, e));
	ok = ok && voe_render_frame_draw_elements(
			   device,
			   voe_render_element_transform(
				   voe_game_interface_surface(size)),
			   first, records);
	voe_render_pass_end(device);
	return ok;
}

bool voe_game_starting_frame(voe_app *app, voe_ui_context *ui,
			     voe_base_arena *frame_arena, const char *line)
{
	voe_platform_size size;
	bool drawing;
	bool drawn;

	VOE_BASE_ASSERT(app != NULL && ui != NULL && frame_arena != NULL &&
				line != NULL,
			"a starting frame with no app, context, arena or line");

	if (!starting_size(app, &size))
		return false;
	if (size.width <= 0 || size.height <= 0)
		return true;
	if (!starting_layout(ui, frame_arena, voe_game_interface_surface(size),
			     line))
		return false;
	if (!voe_app_draw_open(app, size, &drawing))
		return false;
	if (!drawing)
		return true;
	// A refused pass still closes the draw, so the frame ends as render
	// expects.
	drawn = starting_draw(voe_app_device(app), ui, size);
	return voe_app_draw_close(app) && drawn;
}

bool voe_game_starting_prepare(voe_app *app, voe_ui_context *ui,
			       voe_base_arena *frame_arena, const char *line)
{
	VOE_BASE_ASSERT(app != NULL && frame_arena != NULL,
			"a starting prepare with no app or arena");

	while (true) {
		struct voe_base_arena_mark mark = voe_base_arena_mark(frame_arena);
		bool shown = voe_game_starting_frame(app, ui, frame_arena, line);

		voe_base_arena_rewind(frame_arena, mark);
		if (!shown)
			return false;
		switch (voe_render_device_prepare(voe_app_device(app))) {
		case VOE_RENDER_PREPARING:
			break;
		case VOE_RENDER_PREPARED:
			return true;
		case VOE_RENDER_PREPARE_FAILED:
			return false;
		}
	}
}
