// The game's interface on a headless device opened as tests/frame.c opens
// one: the surface of a 1280x720 window is 240x135 millimetres, and a run
// hands back what a stand-in project interface answers. The one answering
// false lays out a panel with a label and a button, ends the frame, and
// leaves element records; the one answering true ends an empty frame; a third
// sets both asks, and after the run the caller's asks read true (0333).
//
// A machine with no usable Vulkan skips and says so.
#include <game/frame.h>
#include <game/interface.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <ui/widgets.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>

// A panel with a label and a button over the whole surface; ends the run.
static bool quitting(const voe_game_project_frame *frame)
{
	voe_ui_column_begin(frame->ui,
			    (voe_ui_container){
				    .size = { .along = { VOE_UI_SIZE_FIXED,
							 frame->size.y },
					      .across = { VOE_UI_SIZE_FIXED,
							  frame->size.x } } });
	voe_ui_panel_begin(frame->ui, "menu", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){ 0 });
	voe_ui_label(frame->ui, "Coins");
	voe_ui_button_begin(frame->ui, "quit", 0);
	voe_ui_label(frame->ui, "Quit");
	voe_ui_end(frame->ui);
	voe_ui_end(frame->ui);
	voe_ui_end(frame->ui);
	VOE_TEST_CHECK(voe_ui_frame_end(frame->ui));
	return false;
}

// An empty frame; the run goes on.
static bool going_on(const voe_game_project_frame *frame)
{
	voe_ui_column_begin(frame->ui, (voe_ui_container){ 0 });
	voe_ui_end(frame->ui);
	VOE_TEST_CHECK(voe_ui_frame_end(frame->ui));
	return true;
}

// An empty frame asking to pause and to start again; the run goes on.
static bool asking(const voe_game_project_frame *frame)
{
	voe_ui_column_begin(frame->ui, (voe_ui_container){ 0 });
	voe_ui_end(frame->ui);
	VOE_TEST_CHECK(voe_ui_frame_end(frame->ui));
	frame->asks->paused = true;
	frame->asks->restart = true;
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 23);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_platform_size size = { 1280, 720 };
	voe_app_settings settings = { .width = 128,
				      .height = 72,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_math_float2 surface = voe_game_interface_surface(size);
	voe_game_project_asks asks = { 0 };
	voe_game_interface *interface;
	voe_app *app;

	VOE_TEST_CHECK_FLOAT(surface.x, 240.0f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(surface.y, 135.0f, 1e-3f);

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
		VOE_TEST_CHECK(!voe_game_interface_run(interface, scratch, NULL,
						       NULL, size, &asks,
						       quitting));
		VOE_TEST_CHECK(voe_ui_element_count(voe_game_interface_context(
				       interface)) > 0);
		voe_base_arena_clear(scratch);
		VOE_TEST_CHECK(voe_game_interface_run(interface, scratch, NULL,
						      NULL, size, &asks,
						      going_on));
		VOE_TEST_CHECK(!asks.paused && !asks.restart);
		voe_base_arena_clear(scratch);
		VOE_TEST_CHECK(voe_game_interface_run(interface, scratch, NULL,
						      NULL, size, &asks,
						      asking));
		VOE_TEST_CHECK(asks.paused);
		VOE_TEST_CHECK(asks.restart);
		voe_game_interface_destroy(interface);
	}

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
