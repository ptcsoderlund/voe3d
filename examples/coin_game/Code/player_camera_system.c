// The camera's step: registers player_camera_state, and after the move adds
// its row or places the scene's camera behind the first player.
//
// The first step reads yaw and pitch off the camera's authored rotation
// (0261 point 2) and the distance off the first player; every later step
// sets the row's arm by the spring arm (0261 point 5, 0262) and submits the
// camera's transform from the row, which player_camera_look turns once a
// frame.
//
// The arm snaps in only when something solid is in the way of where the
// camera now is; otherwise it glides to the wheel's distance both ways. The
// test reaches max(arm, distance), not only the distance, so a camera gliding
// in from beyond a nearer distance never sits inside a wall on the way.
//
// Constraints: a full structural or transform queue leaves the camera where
// it was this step. The runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). The spring arm
// bisects capsule overlaps, about fifteen a step, because physics has no ray
// or sweep yet (0253); one would replace the bisection.
#include "player.h"
#include "player_camera.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <math/float3.h>
#include <math/quat.h>

#include <physics/collider_component.h>
#include <physics/overlap.h>
#include <physics/shape.h>

#include <scene/camera_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stdint.h>

#define PLAYER_CAMERA_CONTACTS 16u
#define PLAYER_CAMERA_ARM_SPEED 10.0f

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

// The row's rotation: yaw about +Y, then pitch about +X.
static voe_math_quat player_camera_rotation_of(const player_camera_state *row)
{
	VOE_BASE_ASSERT(row != NULL, "turning the camera from no row");
	const voe_math_quat rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle((voe_math_float3){ 0, 1, 0 },
					      row->yaw),
		voe_math_quat_from_axis_angle((voe_math_float3){ 1, 0, 0 },
					      row->pitch));

	VOE_BASE_DEBUG_ASSERT(fabsf(voe_math_quat_length(rotation) - 1.0f) <
				      0.01f,
			      "a camera rotation that is not unit");
	return rotation;
}

// Whether a capsule of radius 0.25 m from the player along the rotation's +Z
// for `length` metres touches anything solid. Triggers and the player are not
// in the way.
static bool player_camera_blocked(const voe_ecs_world *world,
				  voe_ecs_entity player_entity,
				  voe_math_double3 from, voe_math_quat rotation,
				  float length)
{
	VOE_BASE_ASSERT(world != NULL && length >= 0.0f,
			"testing a camera arm of no length or in no world");
	const voe_math_float3 back =
		voe_math_float3_scale(player_camera_forward_of(rotation), -1.0f);
	const voe_physics_shape arm = {
		.kind = VOE_PHYSICS_COLLIDER_CAPSULE,
		.centre = voe_math_double3_add(
			from, voe_math_double3_from_float3(voe_math_float3_scale(
				      back, length * 0.5f))),
		.rotation = voe_math_quat_mul(
			rotation, voe_math_quat_from_axis_angle(
					  (voe_math_float3){ 1, 0, 0 },
					  PLAYER_CAMERA_PI * 0.5f)),
		.half = { 0.25f, length * 0.5f + 0.25f, 0.0f },
	};
	voe_physics_contact contacts[PLAYER_CAMERA_CONTACTS];
	const uint32_t found = voe_physics_overlap(
		world, arm, player_entity, contacts, PLAYER_CAMERA_CONTACTS);

	VOE_BASE_ASSERT(found <= PLAYER_CAMERA_CONTACTS, "more contacts than room");
	for (uint32_t i = 0; i < found; i++)
		if (!contacts[i].trigger)
			return true;
	return false;
}

// The longest clear arm up to `distance`: all of it when clear, else
// bisected to 1 cm. Overlap only grows with length, so bisection holds.
static float player_camera_clear_length(const voe_ecs_world *world,
					voe_ecs_entity player_entity,
					voe_math_double3 from,
					voe_math_quat rotation, float distance)
{
	VOE_BASE_ASSERT(distance >= 0.0f, "a camera distance below nought");
	if (!player_camera_blocked(world, player_entity, from, rotation,
				   distance))
		return distance;
	float clear = 0.0f;
	float blocked = distance;

	for (int i = 0; i < 32 && blocked - clear > 0.01f; i++) {
		const float middle = 0.5f * (clear + blocked);

		if (player_camera_blocked(world, player_entity, from, rotation,
					  middle))
			blocked = middle;
		else
			clear = middle;
	}
	VOE_BASE_DEBUG_ASSERT(clear <= distance, "a clear arm past the distance");
	return clear;
}

// The arm after this step (0262): in at once to `clear` when that is shorter
// than the arm, else towards min(distance, clear) by at most
// PLAYER_CAMERA_ARM_SPEED metres a second, nearer or farther alike.
static float player_camera_sprung(float arm, float distance, float clear,
				  double seconds)
{
	VOE_BASE_ASSERT(seconds >= 0.0, "a step of negative seconds");
	if (clear < arm)
		return clear;
	const float wanted = fminf(distance, clear);
	const float step = PLAYER_CAMERA_ARM_SPEED * (float)seconds;
	const float next = fminf(fmaxf(wanted, arm - step), arm + step);

	VOE_BASE_DEBUG_ASSERT(next <= clear + 0.001f,
			      "an arm past what is clear");
	return next;
}

// The camera's transform: the row's rotation, at the target plus the
// rotation's +Z times the arm.
static voe_scene_transform player_camera_placed(voe_scene_transform own,
						voe_math_double3 target,
						const player_camera_state *row)
{
	VOE_BASE_ASSERT(row != NULL, "placing the camera from no row");
	const voe_math_quat rotation = player_camera_rotation_of(row);

	own.rotation = rotation;
	own.position = voe_math_double3_sub(
		target, voe_math_double3_from_float3(voe_math_float3_scale(
				player_camera_forward_of(rotation), row->arm)));
	VOE_BASE_DEBUG_ASSERT(row->arm >= 0.0f, "a camera arm below nought");
	return own;
}

void player_camera_run(voe_ecs_world *world, double seconds)
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
	const voe_ecs_entity player_entity =
		voe_ecs_component_entities(world, type)[0];
	const voe_scene_transform *target =
		voe_ecs_component_get(world, transform_type, player_entity);
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
	player_camera_state row = *state;

	row.arm = player_camera_sprung(
		row.arm, row.distance,
		player_camera_clear_length(world, player_entity,
					   target->position,
					   player_camera_rotation_of(&row),
					   fmaxf(row.arm, row.distance)),
		seconds);
	const bool ok = voe_ecs_component_set(world, state_type, camera, &row);

	VOE_BASE_ASSERT(ok, "a player_camera_state row vanished while springing");
	(void)voe_scene_transform_submit(world, (voe_scene_transform_intent){
		camera, player_camera_placed(*own, target->position, &row) });
}
