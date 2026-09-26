// The camera's step: registers player_camera_state, and after the move adds
// its row or places the scene's camera behind the first player.
//
// The first step reads yaw and pitch off the camera's authored rotation
// (0261 point 2) and the distance off the first player; every later step
// submits the camera's transform from the row, which player_camera_look
// turns once a frame.
//
// Constraints: a full structural or transform queue leaves the camera where
// it was this step. The runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245).
#include "player.h"
#include "player_camera.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <math/float3.h>
#include <math/quat.h>

#include <scene/camera_component.h>
#include <scene/transform_system.h>

#include <math.h>

const struct voe_ecs_key player_camera_state_key = { "player_camera_state" };

bool player_camera_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the camera in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&player_camera_state_key, sizeof(player_camera_state), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

// (0, 0, -1) rotated by the unit quaternion q: the third column of its
// matrix, negated. math/quat.h has no rotate, and this is all of it needed.
static voe_math_float3 player_camera_forward_of(voe_math_quat q)
{
	const voe_math_float3 forward = {
		-2.0f * (q.x * q.z + q.w * q.y),
		-2.0f * (q.y * q.z - q.w * q.x),
		-(1.0f - 2.0f * (q.x * q.x + q.y * q.y)),
	};

	VOE_BASE_DEBUG_ASSERT(voe_math_float3_length(forward) < 1.01f,
			      "a forward longer than one");
	return forward;
}

// The first row, from the camera's rotation and the first player's numbers.
static player_camera_state player_camera_first(voe_math_quat rotation,
					       const player *row)
{
	VOE_BASE_ASSERT(row != NULL, "starting the camera on no player");
	const voe_math_float3 f = player_camera_forward_of(rotation);
	const float distance =
		fminf(fmaxf(row->camera_distance, row->camera_distance_min),
		      row->camera_distance_max);
	const player_camera_state first = {
		.yaw = atan2f(-f.x, -f.z),
		.pitch = fminf(fmaxf(asinf(fminf(fmaxf(f.y, -1.0f), 1.0f)),
				     PLAYER_CAMERA_PITCH_MIN),
			       PLAYER_CAMERA_PITCH_MAX),
		.distance = distance,
		.arm = distance,
	};

	VOE_BASE_ASSERT(first.pitch < 0.0f, "a camera looking up at its player");
	return first;
}

// The camera's transform: rotation yaw then pitch, at the target plus the
// rotation's +Z times the arm.
static voe_scene_transform player_camera_placed(voe_scene_transform own,
						voe_math_double3 target,
						const player_camera_state *row)
{
	VOE_BASE_ASSERT(row != NULL, "placing the camera from no row");
	const voe_math_quat rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle((voe_math_float3){ 0, 1, 0 },
					      row->yaw),
		voe_math_quat_from_axis_angle((voe_math_float3){ 1, 0, 0 },
					      row->pitch));

	own.rotation = rotation;
	own.position = voe_math_double3_sub(
		target, voe_math_double3_from_float3(voe_math_float3_scale(
				player_camera_forward_of(rotation), row->arm)));
	VOE_BASE_DEBUG_ASSERT(row->arm >= 0.0f, "a camera arm below nought");
	return own;
}

void player_camera_run(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "placing the camera in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);
	const voe_ecs_type camera_type =
		voe_ecs_component_type(world, &voe_scene_camera_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const voe_ecs_type state_type =
		voe_ecs_component_type(world, &player_camera_state_key);

	if (voe_ecs_component_count(world, type) == 0 ||
	    voe_ecs_component_count(world, camera_type) == 0)
		return;
	const voe_ecs_entity camera =
		voe_ecs_component_entities(world, camera_type)[0];
	const voe_scene_transform *own =
		voe_ecs_component_get(world, transform_type, camera);
	const voe_scene_transform *target = voe_ecs_component_get(
		world, transform_type, voe_ecs_component_entities(world, type)[0]);
	const player_camera_state *state =
		voe_ecs_component_get(world, state_type, camera);

	if (own == NULL || target == NULL)
		return;
	if (state == NULL) {
		const player_camera_state first = player_camera_first(
			own->rotation,
			&((const player *)voe_ecs_component_rows(world, type))[0]);

		(void)voe_ecs_structure_add(world, state_type, camera, &first);
		return;
	}
	(void)voe_scene_transform_submit(world, (voe_scene_transform_intent){
		camera, player_camera_placed(*own, target->position, state) });
}
