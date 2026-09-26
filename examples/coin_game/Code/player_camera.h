// The coin game's camera: turned by the mouse with the right button held,
// zoomed by the wheel, and placed behind the first player after each move.
//
//     player_camera_register(world);   // in voe_game_project_register
//     player_camera_run(world, seconds); // after the move, each fixed step
//     player_camera_look(frame);       // once a frame, before the screens
//
// player_camera_state is one runtime-only row on the scene's one camera
// (0218, 0261 point 2), added by player_camera_run's first step through the
// structural queue, so it exists from the step after. Never saved, never in
// the Inspector. ONE WRITER, THIS MODULE: player_camera_run adds it and
// player_camera_look turns and zooms it.
//
// THE MOUSE IS READ ONCE A FRAME, THE CAMERA PLACED IN A STEP: motion and the
// wheel drain at each poll while a frame runs zero to four steps, so reading
// them in a step would lose or double them (0261 point 3). The placement is
// in the after-move slot so the camera follows the player in the same step.
//
// `yaw` and `pitch` are radians; the rotation is yaw about +Y then pitch
// about +X, so it never rolls or flips. Pitch stays in [-80 deg, -5 deg],
// always looking down at the player; yaw wraps to (-pi, pi]. `distance` is
// the metres the wheel chose, `arm` the metres the camera sits at; `lock_asked`
// whether the pointer lock was last asked for.
//
// THE ARM SPRINGS IN FRONT OF WHAT IS IN THE WAY (0261 point 5): each step it
// is the longest length up to the distance that a 0.25 m capsule from the
// player to the camera leaves clear. In the way is any solid collider: not a
// trigger, not the player. The zoom glides nearer and farther at 10 m/s; the
// arm comes in at once only for something solid in the way (0262).
//
// Constraints: one row (capacity 1). Nothing turns off the playing phase, and
// the lock is let go there. The arm test keeps 16 contacts, so past 16 triggers
// on it a solid collider may be missed.
#pragma once

#include <ecs/world.h>

#include <game/project.h>

#include <stdbool.h>

#define PLAYER_CAMERA_PI 3.14159265358979323846f
#define PLAYER_CAMERA_PITCH_MIN (-80.0f * PLAYER_CAMERA_PI / 180.0f)
#define PLAYER_CAMERA_PITCH_MAX (-5.0f * PLAYER_CAMERA_PI / 180.0f)

typedef struct {
	float yaw;
	float pitch;
	float distance;
	float arm;
	bool lock_asked;
} player_camera_state;

extern const struct voe_ecs_key player_camera_state_key;

// Registers player_camera_state runtime-only with no menu. False, reported,
// when game refuses it.
[[nodiscard]] bool player_camera_register(voe_ecs_world *world);

// Adds the row on its first run, from the camera's own rotation and the first
// player's camera_distance; after, springs the arm over the step's seconds
// and places the camera arm metres behind the first player along the row's
// rotation.
void player_camera_run(voe_ecs_world *world, double seconds);

// Takes and lets go of the pointer lock, turns by the mouse while the right
// button is held and zooms by the wheel, all only while playing.
void player_camera_look(const voe_game_project_frame *frame);
