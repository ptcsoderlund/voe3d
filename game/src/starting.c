// The starting frame, the prepare loop, the wait on a worker and the shaders
// step, as game/include/game/starting.h gives. The element records reach the pass as frame.c's interface draw sends
// them, less the depth clear: a pass with no camera has no depth to clear.
// The splash is three layers under one surface-sized column: the edge image in
// the flow, then two anchored containers painting over it in call order.
#include <game/starting.h>

#include <game/interface.h>

#include <app/pipeline_cache.h>

#include <base/assert.h>

#include <platform/window.h>

#include <render/device.h>

#include <ui/widgets.h>

#include <math.h>
#include <stdatomic.h>
#include <stdint.h>
#include <threads.h>

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

// The splash's line panel: padding round the line, in millimetres, and its
// lower edge's height above the surface's (0356).
#define SPLASH_PAD 3.0f
#define SPLASH_LIFT 8.0f

// The splash under the root, which is `surface` large: its top-left texel
// over the whole surface, the picture centred at the largest size that keeps
// its aspect, and `line` on a GROUND panel at the bottom middle.
static void starting_splash(voe_ui_context *ui, voe_math_float2 surface,
			    const voe_app_picture *splash, const char *line)
{
	float wide = (float)splash->width;
	float high = (float)splash->height;
	float scale = fminf(surface.x / wide, surface.y / high);
	voe_math_float4 edge = { 0.25f / wide, 0.25f / high, 0.5f / wide,
				 0.5f / high };

	(void)voe_ui_image(ui, splash->texture, edge, (voe_math_float2){ 0, 0 },
			   (voe_ui_sizing){
				   .along = { VOE_UI_SIZE_FIXED, surface.y },
				   .across = { VOE_UI_SIZE_FIXED, surface.x } });
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_FIXED, wide * scale },
				      .across = { VOE_UI_SIZE_FIXED,
						  high * scale } },
			    .anchor = { .anchored = true,
					.x = { VOE_UI_ACROSS_CENTER, 0 },
					.y = { VOE_UI_ACROSS_CENTER, 0 } } });
	(void)voe_ui_image(ui, splash->texture, (voe_math_float4){ 0, 0, 1, 1 },
			   (voe_math_float2){ 0, 0 },
			   (voe_ui_sizing){
				   .along = { VOE_UI_SIZE_FIXED, high * scale },
				   .across = { VOE_UI_SIZE_FIXED, wide * scale } });
	voe_ui_end(ui);
	voe_ui_panel_begin(
		ui, "starting", 0, VOE_UI_SURFACE_GROUND,
		(voe_ui_container){
			.pad = { SPLASH_PAD, SPLASH_PAD, SPLASH_PAD, SPLASH_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_CENTER, 0 },
				    .y = { VOE_UI_ACROSS_END, SPLASH_LIFT } } });
	voe_ui_label(ui, line);
	voe_ui_end(ui);
}

// `line` centred on a GROUND panel as large as `surface`, or the splash
// when there is one. False when the frame was refused.
static bool starting_layout(voe_ui_context *ui, voe_base_arena *frame_arena,
			    voe_math_float2 surface,
			    const voe_app_picture *splash, const char *line)
{
	voe_ui_frame_begin(ui, frame_arena);
	if (splash != NULL) {
		voe_ui_column_begin(
			ui, (voe_ui_container){
				    .size = { .along = { VOE_UI_SIZE_FIXED,
							 surface.y },
					      .across = { VOE_UI_SIZE_FIXED,
							  surface.x } } });
		starting_splash(ui, surface, splash, line);
		voe_ui_end(ui);
		return voe_ui_frame_end(ui);
	}
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
			     voe_base_arena *frame_arena,
			     const voe_app_picture *splash, const char *line)
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
			     splash, line))
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
			       voe_base_arena *frame_arena,
			       const voe_app_picture *splash, const char *line)
{
	VOE_BASE_ASSERT(app != NULL && frame_arena != NULL,
			"a starting prepare with no app or arena");

	while (true) {
		struct voe_base_arena_mark mark = voe_base_arena_mark(frame_arena);
		bool shown = voe_game_starting_frame(app, ui, frame_arena,
						     splash, line);

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

// What the worker is handed and hands back: the work, its answer, and whether
// it has ended, which the main thread reads.
struct starting_worker {
	voe_game_starting_work *work;
	void *context;
	voe_game_progress *progress;
	bool answer;
	atomic_bool ended;
};

static int starting_worker_run(void *argument)
{
	struct starting_worker *worker = argument;

	VOE_BASE_ASSERT(worker != NULL && worker->work != NULL,
			"a starting worker with no work");
	worker->answer = worker->work(worker->context, worker->progress);
	atomic_store(&worker->ended, true);
	return 0;
}

// One frame of the line, the arena rewound after it; a window not visible
// draws none and waits on the window instead. False when the window is closing
// or the frame was false.
static bool starting_shown(voe_app *app, voe_ui_context *ui,
			   voe_base_arena *frame_arena,
			   const voe_app_picture *splash, const char *line)
{
	voe_platform_window *window = voe_app_window(app);
	struct voe_base_arena_mark mark;
	bool shown;

	if (window != NULL && !voe_platform_window_visible(window)) {
		voe_platform_window_wait(window, 0.05);
		voe_platform_window_poll(window);
		return !voe_platform_window_should_close(window);
	}
	mark = voe_base_arena_mark(frame_arena);
	shown = voe_game_starting_frame(app, ui, frame_arena, splash, line);
	voe_base_arena_rewind(frame_arena, mark);
	return shown;
}

bool voe_game_starting_wait(voe_app *app, voe_ui_context *ui,
			    voe_base_arena *frame_arena,
			    const voe_app_picture *splash,
			    voe_game_starting_work *work, void *context)
{
	voe_game_progress progress = { 0 };
	struct starting_worker worker = { .work = work,
					  .context = context,
					  .progress = &progress };
	char line[96];
	thrd_t thread;

	VOE_BASE_ASSERT(app != NULL && ui != NULL && frame_arena != NULL &&
				work != NULL,
			"a starting wait with no app, context, arena or work");

	voe_game_progress_set(&progress, "Starting", 0, 0);
	// No thread to be had: the work runs here, the window unanswered until
	// it ends, which is slow but still a start.
	if (thrd_create(&thread, starting_worker_run, &worker) != thrd_success)
		return work(context, &progress);
	while (!atomic_load(&worker.ended)) {
		voe_game_progress_line(&progress, line, sizeof(line));
		if (!starting_shown(app, ui, frame_arena, splash, line)) {
			atomic_store(&progress.stop, true);
			(void)thrd_join(thread, NULL);
			return false;
		}
	}
	(void)thrd_join(thread, NULL);
	return worker.answer;
}

bool voe_game_starting_shaders(voe_render_device *device,
			       const char *cache_path, voe_base_arena *scratch,
			       voe_game_progress *progress)
{
	uint32_t steps;

	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"a shaders step with no device or scratch");

	steps = voe_render_device_prepare_steps(device);
	voe_app_pipeline_cache_load(device, cache_path, scratch);
	// One more call than the steps: the bound, should the last step answer
	// PREPARING and only the next one PREPARED.
	for (uint32_t done = 0; done <= steps; done++) {
		voe_game_progress_set(progress, "Preparing shaders",
				      done < steps ? done : steps, steps);
		if (voe_game_progress_stopped(progress))
			return false;
		switch (voe_render_device_prepare(device)) {
		case VOE_RENDER_PREPARING:
			break;
		case VOE_RENDER_PREPARED:
			voe_app_pipeline_cache_save(device, cache_path, scratch);
			return true;
		case VOE_RENDER_PREPARE_FAILED:
			return false;
		}
	}
	VOE_BASE_ASSERT(false, "prepare took more steps than it counts");
	return false;
}
