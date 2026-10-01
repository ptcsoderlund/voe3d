// The fixed steps in the order game/include/game/steps.h gives, and the bank
// they are drawn from.
#include <game/steps.h>

#include <game/frame.h>

#include <audio/sound_system.h>

#include <3d/emitter_system.h>
#include <3d/water_system.h>

#include <base/assert.h>

#include <physics/body_system.h>

#include <scene/transform_system.h>

#include <math.h>

// The window's width over its height, the listener's; 1 headless or with no
// height.
static float aspect_of(voe_platform_window *window)
{
	voe_platform_size size;

	if (window == NULL)
		return 1.0f;
	size = voe_platform_window_size(window);
	return size.height > 0 && size.width > 0
		       ? (float)size.width / (float)size.height
		       : 1.0f;
}

// One fixed step of the world.
static void step_once(voe_ecs_world *world, voe_platform_window *window,
		      voe_audio_mixer *audio, const voe_game_prefabs *prefabs,
		      const voe_3d_shapes *shapes,
		      void (*systems)(const voe_game_project_step *),
		      void (*after_move)(const voe_game_project_step *))
{
	const voe_game_project_step step = { world, window, audio, prefabs,
					     VOE_GAME_STEP_SECONDS };

	voe_scene_transform_remember(world);
	systems(&step);
	voe_game_world_step(world, shapes);
	voe_physics_body_system_move(world, (float)VOE_GAME_STEP_SECONDS);
	voe_scene_transform_system_run(world);
	after_move(&step);
	voe_game_world_step(world, shapes);
	voe_3d_emitter_system_run(world, (float)VOE_GAME_STEP_SECONDS);
	voe_3d_water_system_run(world, (float)VOE_GAME_STEP_SECONDS);
	if (audio != NULL)
		voe_audio_sound_system_run(world, audio, aspect_of(window));
}

float voe_game_steps_run(voe_game_steps *steps, voe_ecs_world *world,
			 voe_platform_window *window, voe_audio_mixer *audio,
			 const voe_game_prefabs *prefabs,
			 const voe_3d_shapes *shapes, double elapsed,
			 void (*systems)(const voe_game_project_step *),
			 void (*after_move)(const voe_game_project_step *))
{
	float lag;

	VOE_BASE_ASSERT(steps != NULL && world != NULL && shapes != NULL &&
				systems != NULL && after_move != NULL,
			"fixed steps with no bank, world, shapes or systems");
	VOE_BASE_ASSERT(isfinite(elapsed) && elapsed >= 0.0,
			"fixed steps over a negative or endless time");

	steps->banked += elapsed;
	for (int i = 0; i < VOE_GAME_STEPS_MAX &&
			steps->banked >= VOE_GAME_STEP_SECONDS;
	     i++) {
		step_once(world, window, audio, prefabs, shapes, systems,
			  after_move);
		steps->banked -= VOE_GAME_STEP_SECONDS;
	}
	// Past the maximum: keep the phase, drop the whole steps.
	steps->banked = fmod(steps->banked, VOE_GAME_STEP_SECONDS);

	lag = (float)(1.0 - steps->banked / VOE_GAME_STEP_SECONDS);
	VOE_BASE_ASSERT(lag > 0.0f && lag <= 1.0f, "a lag outside (0, 1]");
	return lag;
}
