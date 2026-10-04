// The run's steps in the order game/include/game/run.h gives, each refusal a
// line on stderr and a 1. Everything is released on every path out; a missing
// sound device is not a refusal but a silent game.
// - The interface runs each frame and may end the run.
// - A restart asked makes the world again in its own arena; the mixer pauses
//   with the run.
#include <game/run.h>

#include <game/frame.h>
#include <game/interface.h>
#include <game/models.h>
#include <game/prefabs.h>
#include <game/project.h>
#include <game/scene.h>
#include <game/starting.h>
#include <game/steps.h>
#include <game/world.h>

#include <3d/models.h>
#include <3d/shape_system.h>

#include <app/app.h>
#include <app/start_log.h>

#include <audio/mixer.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/path.h>
#include <platform/sound.h>

#include <stddef.h>

// The app's arena, the world's own, and the scratch startup and each frame's
// draw system work in. Block sizes, not limits.
#define RUN_ARENA (4u * 1024u * 1024u)
#define RUN_SCRATCH (1u * 1024u * 1024u)

// The ceiling on a frame's step, in seconds (app/clock.h).
#define LONGEST_STEP 0.25

// The folder sounds and models are read from: the running program's
// (ADR-0266, ADR-0277 point 4), or "."
// when the system will not say where the program is.
static const char *sound_folder(voe_base_arena *scratch)
{
	const char *program = voe_platform_path_program(scratch);
	const char *folder =
		program == NULL ? NULL : voe_platform_path_parent(scratch, program);

	return folder == NULL ? "." : folder;
}

// The run's sound: the mixer, and its device, NULL when there is none or it
// went away.
struct run_sound {
	voe_audio_mixer *mixer;
	voe_platform_sound *device;
};

// The run's model store and the folder its files are read from.
struct run_models {
	voe_3d_models *store;
	const char *folder;
};

// The world in `world_arena` cleared, the project's types and the cooked
// scene: at the start and at each restart (0333). NULL, with a line on
// stderr, when the scene does not fit its world.
static voe_ecs_world *run_world_make(voe_base_arena *world_arena)
{
	voe_ecs_world *world;

	voe_base_arena_clear(world_arena);
	world = voe_game_world_new(world_arena);
	voe_game_project_register(world);
	if (!voe_game_scene_build(world)) {
		VOE_BASE_ERROR("game", "the scene does not fit its world");
		return NULL;
	}
	return world;
}

// The frames, until the window is closing or the project's interface ends
// the run. False, with a line on stderr, when one was refused. A device whose
// pump fails is destroyed and the sound's device set to NULL, and the game
// goes on silent. Paused, no step runs and the draw keeps the last lag. The
// start's log gets its last two steps here and is written after the first
// frame.
static bool run_frames(voe_app *app, voe_base_arena *world_arena,
		       voe_ecs_world *world, const voe_3d_shapes *shapes,
		       voe_base_arena *scratch, voe_game_interface *interface,
		       struct run_sound *sound, const struct run_models *models,
		       voe_app_start_log *log)
{
	voe_game_project_asks asks = { 0 };
	voe_game_steps steps = { 0 };
	float lag = 1.0f;
	bool logged = false;

	// The built scene's models, before the first frame. Failures are on
	// stderr and kept as failed entries, here and each frame.
	(void)voe_game_models_update(world, models->store, voe_app_device(app),
				     models->folder, scratch);
	voe_app_start_log_step(log, "models and sound");
	while (true) {
		voe_app_frame frame = voe_app_frame_open(app);

		if (frame.closing)
			return true;
		if (frame.minimised)
			continue;
		if (asks.restart) {
			world = run_world_make(world_arena);
			if (world == NULL)
				return false;
			steps = (voe_game_steps){ 0 };
			(void)voe_game_models_update(world, models->store,
						     voe_app_device(app),
						     models->folder, scratch);
			asks = (voe_game_project_asks){ 0 };
		}
		if (!asks.paused)
			lag = voe_game_steps_run(
				&steps, world, voe_app_window(app),
				sound->mixer, &voe_game_prefabs_cooked, shapes,
				frame.tick.step, voe_game_project_systems_run,
				voe_game_project_systems_after_move);
		(void)voe_game_models_update(world, models->store,
					     voe_app_device(app),
					     models->folder, scratch);
		// The ui frame is laid out in scratch and gone by the next.
		voe_base_arena_clear(scratch);
		if (!voe_game_interface_run(interface, scratch, world,
					    voe_app_window(app), frame.size,
					    &asks, voe_game_project_interface))
			return true;
		voe_audio_mixer_pause(sound->mixer, asks.paused);
		if (sound->device != NULL &&
		    !voe_audio_mixer_pump(sound->mixer, sound->device)) {
			voe_platform_sound_destroy(sound->device);
			sound->device = NULL;
		}
		if (!voe_game_frame(app, world, shapes, models->store, scratch,
				    frame.size, lag,
				    voe_game_interface_context(interface))) {
			VOE_BASE_ERROR("game", "the device stopped drawing");
			return false;
		}
		if (!logged) {
			voe_app_start_log_step(log, "first frame");
			voe_base_arena_clear(scratch);
			// With no path only stderr is written, which cannot
			// fail.
			(void)voe_app_start_log_write(log, "game", NULL, scratch);
			logged = true;
		}
	}
}

int voe_game_run(const char *title, voe_game_window window)
{
	voe_base_arena *arena = voe_base_arena_new(RUN_ARENA);
	voe_base_arena *world_arena = voe_base_arena_new(RUN_ARENA);
	voe_base_arena *scratch = voe_base_arena_new(RUN_SCRATCH);
	voe_app_settings settings = { .width = window.width,
				      .height = window.height,
				      .fullscreen = window.fullscreen,
				      .title = title,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = LONGEST_STEP };
	voe_base_error error = VOE_BASE_OK;
	voe_game_interface *interface;
	voe_app_start_log log;
	struct run_models models;
	struct run_sound sound;
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_app *app;
	int status = 1;

	VOE_BASE_ASSERT(window.width > 0 && window.height > 0,
			"a game window with no width or height");
	voe_app_start_log_begin(&log);
	app = voe_app_new(arena, scratch, settings, &error);
	if (app == NULL) {
		VOE_BASE_ERROR("game", "the window would not open: %s",
			       voe_base_error_string(error));
		goto released;
	}
	// Startup keeps nothing in scratch (app/app.h).
	voe_base_arena_clear(scratch);
	voe_app_start_log_step(&log, "window and device");

	interface = voe_game_interface_new(voe_app_device(app), arena);
	if (interface == NULL) {
		VOE_BASE_ERROR("game", "the interface would not open");
		goto closed;
	}
	voe_app_start_log_step(&log, "interface");

	// Text only, until the mesh pipelines are built (0345). A false here
	// is a closing window or a failed build, already on stderr; the run
	// ends as a closed window ends it.
	if (!voe_game_starting_prepare(app,
				       voe_game_interface_context(interface),
				       scratch, "Starting - preparing shaders...")) {
		status = 0;
		goto unbuilt;
	}
	voe_app_start_log_step(&log, "preparing shaders");

	world = run_world_make(world_arena);
	if (world == NULL)
		goto unbuilt;
	voe_app_start_log_step(&log, "world and scene");
	models.folder = sound_folder(arena);
	models.store = voe_3d_models_new();
	sound.mixer = voe_audio_mixer_new(models.folder);
	// NULL is already reported; the game runs silent.
	sound.device = voe_platform_sound_new();
	if (!voe_3d_shapes_upload(voe_app_device(app), &shapes, &error)) {
		VOE_BASE_ERROR("game", "the shapes do not fit the device: %s",
			       voe_base_error_string(error));
	} else if (run_frames(app, world_arena, world, &shapes, scratch,
			      interface, &sound, &models, &log)) {
		status = 0;
	}
	voe_platform_sound_destroy(sound.device);
	voe_audio_mixer_destroy(sound.mixer);
	voe_3d_models_clear(models.store, voe_app_device(app));
	voe_3d_models_destroy(models.store);
unbuilt:
	voe_game_interface_destroy(interface);
closed:
	voe_app_destroy(app);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(world_arena);
	voe_base_arena_destroy(arena);
	return status;
}
