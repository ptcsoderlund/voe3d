// The run's steps in the order game/include/game/run.h gives, each refusal a
// line on stderr and a 1. Everything is released on every path out.
#include <game/run.h>

#include <game/frame.h>
#include <game/project.h>
#include <game/scene.h>
#include <game/world.h>

#include <3d/shape_system.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <stddef.h>

// The app's and the world's arena, and the scratch startup and each frame's
// draw system work in. Block sizes, not limits.
#define RUN_ARENA (4u * 1024u * 1024u)
#define RUN_SCRATCH (1u * 1024u * 1024u)

// The ceiling on a frame's step, in seconds (app/clock.h).
#define LONGEST_STEP 0.25

// The frames, until the window is closing. False when one was refused.
static bool run_frames(voe_app *app, voe_ecs_world *world,
		       const voe_3d_shapes *shapes, voe_base_arena *scratch)
{
	while (true) {
		voe_app_frame frame = voe_app_frame_open(app);

		if (frame.closing)
			return true;
		if (frame.minimised)
			continue;
		voe_game_project_systems_run(&(voe_game_project_step){
			world, voe_app_window(app), frame.tick.step });
		if (!voe_game_frame(app, world, shapes, scratch, frame.size,
				    0.0f))
			return false;
	}
}

int voe_game_run(const char *title)
{
	voe_base_arena *arena = voe_base_arena_new(RUN_ARENA);
	voe_base_arena *scratch = voe_base_arena_new(RUN_SCRATCH);
	voe_app_settings settings = { .width = VOE_GAME_WIDTH,
				      .height = VOE_GAME_HEIGHT,
				      .title = title,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = LONGEST_STEP };
	voe_base_error error = VOE_BASE_OK;
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_app *app;
	int status = 1;

	app = voe_app_new(arena, scratch, settings, &error);
	if (app == NULL) {
		VOE_BASE_ERROR("game", "the window would not open: %s",
			       voe_base_error_string(error));
		goto released;
	}
	// Startup keeps nothing in scratch (app/app.h).
	voe_base_arena_clear(scratch);

	world = voe_game_world_new(arena);
	voe_game_project_register(world);
	if (!voe_game_scene_build(world)) {
		VOE_BASE_ERROR("game", "the scene does not fit its world");
	} else if (!voe_3d_shapes_upload(voe_app_device(app), &shapes,
					 &error)) {
		VOE_BASE_ERROR("game", "the shapes do not fit the device: %s",
			       voe_base_error_string(error));
	} else if (!run_frames(app, world, &shapes, scratch)) {
		VOE_BASE_ERROR("game", "the device stopped drawing");
	} else {
		status = 0;
	}
	voe_app_destroy(app);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return status;
}
