// The player module: registers player and player_state, moves every player
// before the move and places the camera behind the first one after it.
//
// A player's first step only records its state: its position as the start
// and the current restart count, through the structural queue, so the row
// exists from the next step. When game_state's restart count moves on, the
// player is put back at its start, rotation kept, and its body stopped.
// Otherwise it walks as the capsule's player does: W/A/S/D, W along −Z;
// on the floor the Y velocity starts at √(2 · gravity · height) on Space's
// edge, else 0; in the air at the body's own Y; gravity is then taken off.
// Off the playing phase there is no walk and no jump (0260 point 4).
//
// The camera is the scene's one camera (0218), placed after the move so it
// follows the player in the same step (0257).
//
// Constraints: the only writer of player_state rows. A full body, transform
// or structural queue drops the rest of this step's players. The runtime-only
// marker is taken by address at run time, because a project library imports
// it on Windows (0245).
#include "game_state.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>
#include <game/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <physics/body_system.h>

#include <platform/input.h>

#include <scene/camera_component.h>
#include <scene/transform_system.h>

#include <math.h>

const struct voe_ecs_key player_key = { "player" };
const struct voe_ecs_key player_state_key = { "player_state" };

static const player player_default = {
	.speed = 2.0f,
	.jump_height = 1.2f,
	.gravity = 9.81f,
	.camera_distance = 6.0f,
	.start_score = 1000,
	.score_drop = 10,
};

bool player_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering player in no world");
	const bool player_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&player_key, sizeof(player), VOE_GAME_WORLD_AUTHORED,
			VOE_GAME_PROJECT_DESCRIPTION(player), &player_default,
			"Player" });
	const bool state_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&player_state_key, sizeof(player_state),
			VOE_GAME_WORLD_AUTHORED, &voe_ecs_runtime_only, NULL,
			NULL });

	return player_ok && state_ok;
}

// What the keys ask of one step; all nought with no window.
typedef struct {
	float right;
	float forward;
	bool space;
} player_keys;

// -1, 0 or 1 from a pair of opposite keys.
static float player_key_axis(voe_platform_window *window,
			     voe_platform_key minus, voe_platform_key plus)
{
	VOE_BASE_ASSERT(window != NULL, "reading keys from no window");
	const float axis =
		(voe_platform_input_key_down(window, plus) ? 1.0f : 0.0f) -
		(voe_platform_input_key_down(window, minus) ? 1.0f : 0.0f);

	VOE_BASE_ASSERT(axis >= -1.0f && axis <= 1.0f, "an axis past one");
	return axis;
}

static player_keys player_keys_read(voe_platform_window *window)
{
	if (window == NULL)
		return (player_keys){ 0 };
	return (player_keys){
		player_key_axis(window, VOE_PLATFORM_KEY_A, VOE_PLATFORM_KEY_D),
		player_key_axis(window, VOE_PLATFORM_KEY_S, VOE_PLATFORM_KEY_W),
		voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) };
}

// The body this step wants: walk and jump only when `playing`.
static voe_physics_body player_body_wanted(const player *row,
					   const voe_physics_body *body,
					   player_keys keys, bool jump,
					   bool playing, float seconds)
{
	VOE_BASE_ASSERT(row != NULL && body != NULL, "moving no player");
	voe_physics_body wanted = *body;
	float rise = body->velocity.y;

	if (body->on_floor)
		rise = playing && jump ?
			       sqrtf(2.0f * fmaxf(row->gravity, 0.0f) *
				     fmaxf(row->jump_height, 0.0f)) :
			       0.0f;
	wanted.velocity.x = playing ? keys.right * row->speed : 0.0f;
	wanted.velocity.z = playing ? -keys.forward * row->speed : 0.0f;
	wanted.velocity.y = rise - row->gravity * seconds;
	return wanted;
}

// Puts the player back at its start, stopped. False when a queue is full.
static bool player_restart(voe_ecs_world *world, voe_ecs_entity entity,
			   const voe_scene_transform *transform,
			   const voe_physics_body *body,
			   const player_state *state)
{
	VOE_BASE_ASSERT(transform != NULL && body != NULL && state != NULL,
			"restarting no player");
	voe_scene_transform moved = *transform;
	voe_physics_body stopped = *body;

	moved.position = state->start;
	stopped.velocity = (voe_math_float3){ 0 };
	return voe_scene_transform_submit(world, (voe_scene_transform_intent){
						     entity, moved }) &&
	       voe_physics_body_submit(world, (voe_physics_body_intent){
						     entity, stopped });
}

// One player's step. False when a queue is full.
static bool player_step(voe_ecs_world *world, voe_ecs_entity entity,
			const player *row, player_keys keys, double seconds)
{
	const voe_ecs_type state_type =
		voe_ecs_component_type(world, &player_state_key);
	const game_state *game = game_state_get(world);
	const uint32_t restarts = game != NULL ? game->restarts : 0;
	const bool playing = game != NULL && game->phase == GAME_PHASE_PLAYING;
	const voe_scene_transform *transform = voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_transform_key),
		entity);
	const voe_physics_body *body = voe_physics_body_get(world, entity);
	const player_state *state =
		voe_ecs_component_get(world, state_type, entity);

	if (transform == NULL || body == NULL)
		return true;
	if (state == NULL)
		return voe_ecs_structure_add(world, state_type, entity,
			&(player_state){ transform->position, restarts,
					 keys.space });
	// A copy: the set below overwrites the row `state` points at.
	const player_state last = *state;
	player_state next = last;

	next.restarts = restarts;
	next.jump_down = keys.space;
	const bool ok = voe_ecs_component_set(world, state_type, entity, &next);

	VOE_BASE_ASSERT(ok, "a player_state row vanished while stepping it");
	if (last.restarts != restarts)
		return player_restart(world, entity, transform, body, &next);
	return voe_physics_body_submit(world, (voe_physics_body_intent){
		entity, player_body_wanted(row, body, keys,
					   keys.space && !last.jump_down,
					   playing, (float)seconds) });
}

void player_system_run(voe_ecs_world *world, voe_platform_window *window,
		       double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "moving players in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "moving players back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);
	const player *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);
	const player_keys keys = player_keys_read(window);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more player rows than were registered");
	for (uint32_t i = 0; i < count; i++)
		if (!player_step(world, entities[i], &rows[i], keys, seconds))
			return;
}

// (0, 0, -1) rotated by the unit quaternion q: the third column of its
// matrix, negated. math/quat.h has no rotate, and this is all of it needed.
static voe_math_float3 player_forward_of(voe_math_quat q)
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

void player_camera_run(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "placing the camera in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);
	const voe_ecs_type camera_type =
		voe_ecs_component_type(world, &voe_scene_camera_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);

	if (voe_ecs_component_count(world, type) == 0 ||
	    voe_ecs_component_count(world, camera_type) == 0)
		return;
	const voe_ecs_entity camera =
		voe_ecs_component_entities(world, camera_type)[0];
	const voe_scene_transform *own =
		voe_ecs_component_get(world, transform_type, camera);
	const voe_scene_transform *target = voe_ecs_component_get(
		world, transform_type, voe_ecs_component_entities(world, type)[0]);

	if (own == NULL || target == NULL)
		return;
	voe_scene_transform moved = *own;

	moved.position = voe_math_double3_sub(
		target->position,
		voe_math_double3_from_float3(voe_math_float3_scale(
			player_forward_of(own->rotation),
			((const player *)voe_ecs_component_rows(world, type))[0]
				.camera_distance)));
	// A full transform queue leaves the camera where it was this step.
	(void)voe_scene_transform_submit(world, (voe_scene_transform_intent){
						     camera, moved });
}
