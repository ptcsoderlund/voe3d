// The sound system: the one place sound and sound voice rows change, and what
// plays the sounds things carry through the mixer (0304 point 7).
//
//     voe_scene_transform_register(world, capacity);
//     voe_scene_camera_register(world, 1);
//     voe_audio_sound_register(world, capacity);
//     ...
//     voe_audio_sound_system_run(world, mixer, aspect);    // last, each step
//     voe_audio_sound_system_run(world, NULL, 1.0f);       // the editor
//
// A RUN, IN ORDER:
// - it drains the replaces: a dead entity or one with no sound is dropped; a
//   path with no NUL is ended and reported, as the emitter's texture
//   (3d/emitter_system.h), the first of a run named and the rest counted;
// - it drains the controls: play sets playing and the restart flag, stop
//   clears playing, tune sets volume and pitch;
// - it adds a voice row to each sound lacking one and drops the rows whose
//   sound is gone;
// - with a mixer only: the listener is the first camera whose entity has a
//   transform, at its world position, its local +X and -Z turned by the world
//   rotation, tan(fov_y / 2) * aspect wide (audio/place.h); none without one;
// - each row: a one-shot whose voice ended clears playing; restart, or playing
//   with no live voice, starts one (held, loop, volume, pitch, placed at the
//   world position when the entity has a transform); not playing stops it;
//   else it is tuned and moved, which keeps it past the sweep;
// - then the mixer's sweep, so a voice whose thing is gone stops.
//
// A NULL MIXER ONLY DRAINS AND ADDS: the editor applies Inspector edits so and
// plays nothing (0266 point 6). Restart flags wait for a run with a mixer.
//
// Constraints: the report's run and count are per process, as the emitter's
// are. A path or loop changed while its voice plays is heard at the next
// start. An empty or failing path is tried each run; the mixer answers it at
// once and reports a failure once.
#pragma once

#include <audio/mixer.h>
#include <audio/sound_component.h>

#include <ecs/world.h>

// Drains both intents, adds and drops voice rows and, with a mixer, plays.
// Asserts aspect > 0 and the sound registered; with a mixer, the transform
// and camera too.
void voe_audio_sound_system_run(voe_ecs_world *world, voe_audio_mixer *mixer,
				float aspect);
