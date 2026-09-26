// The run's steps in the order game/include/game/run.h gives, each refusal a
// line on stderr and a 1. Everything is released on every path out; a missing
// sound device is not a refusal but a silent game.
#include <game/run.h>

#include <game/frame.h>
#include <game/interface.h>
#include <game/project.h>
#include <game/scene.h>
#include <game/steps.h>
#include <game/world.h>

#include <3d/shape_system.h>

#include <app/app.h>

#include <audio/mixer.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/path.h>
#include <platform/sound.h>

#include <stddef.h>

// The app's and the world's arena, and the scratch startup and each frame's
// draw system work in. Block sizes, not limits.
#define RUN_ARENA (4u * 1024u * 1024u)
#define RUN_SCRATCH (1u * 1024u * 1024u)

// The ceiling on a frame's step, in seconds (app/clock.h).
#define LONGEST_STEP 0.25

// The folder sounds are read from: the running program's (ADR-0266), or "."
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

// The frames, until the window is closing or the project's interface ends
// the run. False when one was refused. A device whose pump fails is destroyed
// and the sound's device set to NULL, and the game goes on silent.
static bool run_frames(voe_app *app, voe_ecs_world *world,
		       const voe_3d_shapes *shapes, voe_base_arena *scratch,
		       voe_game_interface *interface, struct run_sound *sound)
{
	voe_game_steps steps = { 0 };

	while (true) {
		voe_app_frame frame = voe_app_frame_open(app);
		float lag;

		if (frame.closing)
			return true;
		if (frame.minimised)
			continue;
		lag = voe_game_steps_run(&steps, world, voe_app_window(app),
					 sound->mixer, shapes, frame.tick.step,
					 voe_game_project_systems_run,
					 voe_game_project_systems_after_move);
		// The ui frame is laid out in scratch and gone by the next.
		voe_base_arena_clear(scratch);
		if (!voe_game_interface_run(interface, scratch, world,
					    voe_app_window(app), frame.size,
					    voe_game_project_interface))
			return true;
		if (sound->device != NULL &&
		    !voe_audio_mixer_pump(sound->mixer, sound->device)) {
			voe_platform_sound_destroy(sound->device);
			sound->device = NULL;
		}
		if (!voe_game_frame(app, world, shapes, scratch, frame.size,
				    lag, voe_game_interface_context(interface)))
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
	voe_game_interface *interface;
	struct run_sound sound;
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

	interface = voe_game_interface_new(voe_app_device(app), arena);
	if (interface == NULL) {
		VOE_BASE_ERROR("game", "the interface would not open");
		goto closed;
	}

	world = voe_game_world_new(arena);
	voe_game_project_register(world);
	sound.mixer = voe_audio_mixer_new(sound_folder(scratch));
	// NULL is already reported; the game runs silent.
	sound.device = voe_platform_sound_new();
	if (!voe_game_scene_build(world)) {
		VOE_BASE_ERROR("game", "the scene does not fit its world");
	} else if (!voe_3d_shapes_upload(voe_app_device(app), &shapes,
					 &error)) {
		VOE_BASE_ERROR("game", "the shapes do not fit the device: %s",
			       voe_base_error_string(error));
	} else if (!run_frames(app, world, &shapes, scratch, interface,
			       &sound)) {
		VOE_BASE_ERROR("game", "the device stopped drawing");
	} else {
		status = 0;
	}
	voe_platform_sound_destroy(sound.device);
	voe_audio_mixer_destroy(sound.mixer);
	voe_game_interface_destroy(interface);
closed:
	voe_app_destroy(app);
released:
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	return status;
}
