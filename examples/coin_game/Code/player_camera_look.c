// The camera's once-a-frame read of the mouse: the pointer lock, the turn
// by motion while the right button is held and the zoom by the wheel.
//
// Called by the interface entry point before the screens (0261 point 3), so
// each poll's motion and wheel are counted exactly once. It writes its own
// row directly; player_camera_run places the camera from it next step.
//
// Constraints: headless (no window), before the row is made or before
// game_state exists, it does nothing. The lock is asked for only when what
// is wanted changes, since each ask is a request to the window system.
#include "game_state.h"
#include "player.h"
#include "player_camera.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <platform/input.h>

#include <math.h>

// Each wheel notch towards the person multiplies the distance by this.
#define PLAYER_CAMERA_ZOOM_STEP 1.15f

// The row turned by `motion` at `degrees` a unit: mouse right looks right,
// mouse up looks up; pitch clamped, yaw wrapped to (-pi, pi].
static player_camera_state player_camera_turned(player_camera_state row,
						voe_platform_motion motion,
						float degrees)
{
	const float radians = degrees * PLAYER_CAMERA_PI / 180.0f;
	float yaw = remainderf(row.yaw - motion.x * radians,
			       2.0f * PLAYER_CAMERA_PI);

	if (yaw <= -PLAYER_CAMERA_PI)
		yaw += 2.0f * PLAYER_CAMERA_PI;
	row.yaw = yaw;
	row.pitch = fminf(fmaxf(row.pitch - motion.y * radians,
				PLAYER_CAMERA_PITCH_MIN),
			  PLAYER_CAMERA_PITCH_MAX);
	VOE_BASE_ASSERT(row.pitch < 0.0f, "a camera looking up at its player");
	VOE_BASE_DEBUG_ASSERT(row.yaw > -PLAYER_CAMERA_PI - 1e-4f &&
				      row.yaw <= PLAYER_CAMERA_PI + 1e-4f,
			      "a yaw past a half turn");
	return row;
}

// The distance zoomed by `notches`, towards the person farther, clamped.
static float player_camera_zoomed(float distance, float notches,
				  const player *numbers)
{
	VOE_BASE_ASSERT(numbers != NULL, "zooming with no player");
	const float zoomed =
		fminf(fmaxf(distance * powf(PLAYER_CAMERA_ZOOM_STEP, notches),
			    numbers->camera_distance_min),
		      numbers->camera_distance_max);

	VOE_BASE_DEBUG_ASSERT(zoomed == zoomed, "a zoom to no number");
	return zoomed;
}

void player_camera_look(const voe_game_project_frame *frame)
{
	VOE_BASE_ASSERT(frame != NULL && frame->world != NULL,
			"looking around in no frame");
	voe_ecs_world *world = frame->world;
	const voe_ecs_type type =
		voe_ecs_component_type(world, &player_camera_state_key);
	const voe_ecs_type player_type =
		voe_ecs_component_type(world, &player_key);
	const game_state *game = game_state_get(world);

	if (frame->window == NULL || game == NULL ||
	    voe_ecs_component_count(world, type) == 0 ||
	    voe_ecs_component_count(world, player_type) == 0)
		return;
	const voe_ecs_entity camera = voe_ecs_component_entities(world, type)[0];
	const player *numbers = voe_ecs_component_rows(world, player_type);
	const bool playing = game->phase == GAME_PHASE_PLAYING;
	const bool lock = playing && voe_platform_input_button_down(
					     frame->window,
					     VOE_PLATFORM_BUTTON_RIGHT);
	player_camera_state row =
		((const player_camera_state *)voe_ecs_component_rows(world,
								     type))[0];

	if (lock != row.lock_asked)
		voe_platform_input_lock_pointer(frame->window, lock);
	row.lock_asked = lock;
	if (lock)
		row = player_camera_turned(
			row, voe_platform_input_motion(frame->window),
			numbers->camera_turn_speed);
	if (playing) {
		row.distance = player_camera_zoomed(
			row.distance,
			voe_platform_input_wheel(frame->window).y, numbers);
	}
	const bool ok = voe_ecs_component_set(world, type, camera, &row);

	VOE_BASE_ASSERT(ok, "a player_camera_state row vanished while looking");
}
