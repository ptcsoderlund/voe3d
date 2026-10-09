// The run's steps in the order game/include/game/run.h gives, each refusal a
// line on stderr and a 1. Everything is released on every path out; a missing
// sound device is not a refusal but a silent game.
// - WHICH THREAD DOES WHAT (0370 point 4). The main thread opens the window,
//   the interface and the splash, then only polls and draws splash frames
//   while one worker (game/starting.h) prepares the shaders, makes the world
//   and scene, uploads the shapes and reads the models, in its own scratch,
//   never the main thread's. Once it is joined, the main thread alone has the
//   world, the store and the log: it gives the splash back, opens the mixer
//   and sound device, runs the frames and writes the log.
// - The interface runs each frame and may end the run.
// - A restart asked makes the world again in its own arena; the mixer pauses
//   with the run.
// - The window's remembered bounce casters are kept for the whole run, across
//   frames and restarts.
// - The only file naming the project's cooked C: voe_game_scene_build,
//   voe_game_prefabs_cooked and voe_game_landscapes_cooked, the landscapes
//   loaded before the first models; and the project's entry points.
#include <game/run.h>

#include <game/frame.h>
#include <game/interface.h>
#include <game/landscapes.h>
#include <game/models.h>
#include <game/prefabs.h>
#include <game/progress.h>
#include <game/project.h>
#include <game/scene.h>
#include <game/starting.h>
#include <game/steps.h>
#include <game/world.h>

#include <3d/bounce_casters.h>
#include <3d/models.h>
#include <3d/shape_system.h>

#include <app/app.h>
#include <app/picture.h>
#include <app/pipeline_cache.h>
#include <app/start_log.h>

#include <audio/mixer.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/path.h>
#include <platform/sound.h>

#include <stddef.h>

// The app's arena, the world's own, the scratch startup and each frame's
// draw system work in, and the start's worker's own scratch. Block sizes, not
// limits.
#define RUN_ARENA (4u * 1024u * 1024u)
#define RUN_SCRATCH (1u * 1024u * 1024u)

// The ceiling on a frame's step, in seconds (app/clock.h).
#define LONGEST_STEP 0.25

// The folder the splash, sounds and models are read from: the running program's
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

// What the start's worker is handed and what it makes. The worker owns it
// during the wait; the main thread reads it only once the wait has joined.
// `failed` tells a failed step, already on stderr, from a stop.
struct run_start {
	voe_render_device *device;
	voe_base_arena *scratch;
	voe_base_arena *world_arena;
	const char *folder;
	voe_app_start_log *log;
	voe_ecs_world *world;
	voe_3d_shapes shapes;
	voe_3d_models *store;
	bool failed;
};

// The start's work, on the wait's worker (0370 point 4): the shaders with the
// game's cache, the world and scene, the shapes, the cooked landscapes and the
// scene's models, each
// part's log step taken where it ends. False once stopped, or with `failed`
// set on a failed step. Only the worker's own scratch is used, kept empty.
static bool run_start_work(void *context, voe_game_progress *progress)
{
	struct run_start *start = context;
	voe_base_error error = VOE_BASE_OK;
	const char *cache = voe_app_pipeline_cache_path(start->scratch,
							"pipelines_game.cache");
	bool prepared = voe_game_starting_shaders(start->device, cache,
						  start->scratch, progress);

	voe_base_arena_clear(start->scratch);
	if (voe_game_progress_stopped(progress))
		return false;
	if (!prepared) {
		VOE_BASE_ERROR("game", "the shaders would not prepare");
		start->failed = true;
		return false;
	}
	voe_app_start_log_step(start->log, "preparing shaders");
	voe_game_progress_set(progress, "Loading scene", 0, 0);
	start->world = run_world_make(start->world_arena);
	if (start->world == NULL) {
		start->failed = true;
		return false;
	}
	voe_app_start_log_step(start->log, "world and scene");
	if (voe_game_progress_stopped(progress))
		return false;
	if (!voe_3d_shapes_upload(start->device, &start->shapes, &error)) {
		VOE_BASE_ERROR("game", "the shapes do not fit the device: %s",
			       voe_base_error_string(error));
		start->failed = true;
		return false;
	}
	start->store = voe_3d_models_new();
	// Failures are on stderr and kept as failed entries. The cooked
	// landscapes go first, so the scene's landscape rows find them held.
	(void)voe_game_models_landscapes(start->store, start->device,
					 &voe_game_landscapes_cooked,
					 start->scratch);
	(void)voe_game_models_update(start->world, start->store, start->device,
				     start->folder, start->scratch, progress);
	if (voe_game_progress_stopped(progress))
		return false;
	voe_app_start_log_step(start->log, "shapes and models");
	return true;
}

// The wait on the start's worker over splashscreen.png in `start`'s folder,
// the plain screen with a line on stderr when it will not read; the texture
// is given back once the wait has ended, as a game changes no scene yet
// (0356). The frames lay out in scratch, rewound after. The wait's answer.
static bool run_starting(voe_app *app, voe_game_interface *interface,
			 voe_base_arena *scratch, struct run_start *start)
{
	const char *path = voe_platform_path_join(scratch, start->folder,
						  "splashscreen.png");
	voe_base_error error = VOE_BASE_OK;
	voe_app_picture splash;
	bool read;
	bool started;

	VOE_BASE_ASSERT(app != NULL && interface != NULL && start != NULL,
			"a starting with no app, interface or start");
	read = voe_app_picture_read(voe_app_device(app), path, scratch,
				    &splash, &error);
	if (!read)
		VOE_BASE_ERROR("game", "no splash at %s: %s", path,
			       voe_base_error_string(error));
	started = voe_game_starting_wait(app,
					 voe_game_interface_context(interface),
					 scratch, read ? &splash : NULL,
					 run_start_work, start);
	voe_base_arena_clear(scratch);
	if (read && !voe_render_texture_destroy(voe_app_device(app),
						splash.texture))
		VOE_BASE_ERROR("game", "the splash's texture was already gone");
	return started;
}

// The frames, until the window is closing or the project's interface ends
// the run. False, with a line on stderr, when one was refused. A device whose
// pump fails is destroyed and the sound's device set to NULL, and the game
// goes on silent. Paused, no step runs and the draw keeps the last lag. The
// start's log gets its last step here and is written after the first frame.
// `casters` is the window's bounce memory, kept across frames and restarts.
static bool run_frames(voe_app *app, voe_base_arena *world_arena,
		       voe_ecs_world *world, const voe_3d_shapes *shapes,
		       voe_base_arena *scratch, voe_game_interface *interface,
		       struct run_sound *sound, const struct run_models *models,
		       voe_3d_bounce_casters *casters, voe_app_start_log *log)
{
	voe_game_project_asks asks = { 0 };
	voe_game_steps steps = { 0 };
	float lag = 1.0f;
	bool logged = false;

	// Failures are on stderr and kept as failed entries, each frame.
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
						     models->folder, scratch,
						     NULL);
			asks =(voe_game_project_asks){ 0 };
		}
		if (!asks.paused)
			lag = voe_game_steps_run(
				&steps, world, voe_app_window(app),
				sound->mixer, &voe_game_prefabs_cooked, shapes,
				frame.tick.step, voe_game_project_systems_run,
				voe_game_project_systems_after_move);
		(void)voe_game_models_update(world, models->store,
					     voe_app_device(app),
					     models->folder, scratch, NULL);
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
				    frame.size, lag, casters,
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
	voe_base_arena *worker_scratch = voe_base_arena_new(RUN_SCRATCH);
	voe_app_settings settings = { .width = window.width,
				      .height = window.height,
				      .fullscreen = window.fullscreen,
				      .title = title,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = LONGEST_STEP };
	voe_base_error error = VOE_BASE_OK;
	voe_game_interface *interface;
	voe_3d_bounce_casters *casters;
	voe_app_start_log log;
	struct run_start start;
	struct run_models models;
	struct run_sound sound;
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

	// The start's work on a worker behind the splash beside the program
	// (0356, 0370). A closing window ends the run as a close does; a failed
	// step, already on stderr, is a 1. Whatever the worker made is released.
	models.folder = sound_folder(arena);
	start = (struct run_start){ .device = voe_app_device(app),
				    .scratch = worker_scratch,
				    .world_arena = world_arena,
				    .folder = models.folder,
				    .log = &log };
	if (run_starting(app, interface, scratch, &start)) {
		models.store = start.store;
		sound.mixer = voe_audio_mixer_new(models.folder);
		// NULL is already reported; the game runs silent.
		sound.device = voe_platform_sound_new();
		voe_app_start_log_step(&log, "sound");
		// The window's bounce memory (0394 point 4), zeroed by the push
		// and in the run's arena for as long as the window lives: 20 KB,
		// too much for a small stack.
		casters = voe_base_arena_push(arena, sizeof(*casters));
		if (run_frames(app, world_arena, start.world, &start.shapes,
			       scratch, interface, &sound, &models, casters,
			       &log))
			status = 0;
		voe_platform_sound_destroy(sound.device);
		voe_audio_mixer_destroy(sound.mixer);
	} else if (!start.failed) {
		status = 0;
	}
	if (start.store != NULL) {
		voe_3d_models_clear(start.store, voe_app_device(app));
		voe_3d_models_destroy(start.store);
	}
	voe_game_interface_destroy(interface);
closed:
	voe_app_destroy(app);
released:
	voe_base_arena_destroy(worker_scratch);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(world_arena);
	voe_base_arena_destroy(arena);
	return status;
}
