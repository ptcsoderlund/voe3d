// The coin module: registers coin and the runtime-only coin_taken, and takes
// and puts back coins before the move.
//
// A take is asked, not told (0253 point 5): the coin overlaps its own
// trigger's shape against the world and looks for an entity with a player
// row. The take removes the Shape and adds coin_taken { that shape, the
// current restart count } and plays the coin's sound, once, when both are
// queued; a restart's bump makes the row older, and the
// next step adds the shape back and removes the row. Every change goes
// through the structural queue (0193), so it lands after the systems and no
// system sees a coin half taken.
//
// Constraints: a box collider is not a query shape (physics/overlap.h), so a
// coin with one is never taken; a player past the first COIN_CONTACTS
// contacts is not seen that step; a full queue drops the rest of this step's
// coins. The runtime-only marker is taken by address at run time, because a
// project library imports it on Windows (0245).
#include "coin.h"
#include "game_state.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>
#include <game/world.h>

#include <physics/collider_component.h>
#include <physics/overlap.h>
#include <physics/shape.h>

#include <scene/transform_component.h>

#define COIN_CONTACTS 8u

const struct voe_ecs_key coin_key = { "coin" };
const struct voe_ecs_key coin_taken_key = { "coin_taken" };

static const coin coin_default = { .points = 100, .sound = "pickup.wav" };

bool coin_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering coin in no world");
	const bool coin_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&coin_key, sizeof(coin), VOE_GAME_WORLD_AUTHORED,
			VOE_GAME_PROJECT_DESCRIPTION(coin), &coin_default,
			"Coin" });
	const bool taken_ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&coin_taken_key, sizeof(coin_taken),
			VOE_GAME_WORLD_AUTHORED, &voe_ecs_runtime_only, NULL,
			NULL });

	return coin_ok && taken_ok;
}

// True when the coin's own shape overlaps an entity with a player row.
static bool coin_touched_by_player(const voe_ecs_world *world,
				   voe_ecs_entity self)
{
	VOE_BASE_ASSERT(world != NULL, "asking about a coin in no world");
	const voe_ecs_type player_type =
		voe_ecs_component_type(world, &player_key);
	voe_physics_contact contacts[COIN_CONTACTS];
	voe_physics_shape shape;

	if (!voe_physics_shape_of(world, self, &shape) ||
	    shape.kind == VOE_PHYSICS_COLLIDER_BOX)
		return false;
	const uint32_t found = voe_physics_overlap(world, shape, self,
						   contacts, COIN_CONTACTS);

	VOE_BASE_ASSERT(found <= COIN_CONTACTS, "more contacts than room");
	for (uint32_t i = 0; i < found; i++)
		if (voe_ecs_component_get(world, player_type,
					  contacts[i].entity) != NULL)
			return true;
	return false;
}

// Adds back the shape a coin wore, if it wore one, and removes its mark.
// False when the queue is full.
static bool coin_put_back(voe_ecs_world *world, voe_ecs_entity entity,
			  const coin_taken *taken)
{
	VOE_BASE_ASSERT(world != NULL && taken != NULL, "putting back no coin");
	const voe_ecs_type shape_type =
		voe_ecs_component_type(world, &voe_3d_shape_key);
	const voe_ecs_type taken_type =
		voe_ecs_component_type(world, &coin_taken_key);

	if (taken->shape.kind != 0 &&
	    !voe_ecs_structure_add(world, shape_type, entity, &taken->shape))
		return false;
	return voe_ecs_structure_remove(world, taken_type, entity);
}

// Hides the coin, marks it taken under `restarts` and plays its sound. False
// when the queue is full; the sound waits for the take that is queued.
static bool coin_take(voe_ecs_world *world, voe_audio_mixer *audio,
		      voe_ecs_entity entity, uint32_t restarts)
{
	VOE_BASE_ASSERT(world != NULL && audio != NULL,
			"taking a coin in no world or with no mixer");
	const coin *row = voe_ecs_component_get(
		world, voe_ecs_component_type(world, &coin_key), entity);
	const voe_ecs_type shape_type =
		voe_ecs_component_type(world, &voe_3d_shape_key);
	const voe_ecs_type taken_type =
		voe_ecs_component_type(world, &coin_taken_key);
	const voe_3d_shape *shape =
		voe_ecs_component_get(world, shape_type, entity);
	const coin_taken taken = { shape != NULL ? *shape : (voe_3d_shape){ 0 },
				   restarts };

	VOE_BASE_ASSERT(shape == NULL || taken.shape.kind == shape->kind,
			"a mark that forgot the coin's shape");

	VOE_BASE_ASSERT(row != NULL, "taking an entity that is no coin");

	if (shape != NULL &&
	    !voe_ecs_structure_remove(world, shape_type, entity))
		return false;
	if (!voe_ecs_structure_add(world, taken_type, entity, &taken))
		return false;
	voe_audio_mixer_play(audio, row->sound);
	return true;
}

void coin_system_run(voe_ecs_world *world, voe_audio_mixer *audio)
{
	VOE_BASE_ASSERT(world != NULL, "taking coins in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &coin_key);
	const voe_ecs_type taken_type =
		voe_ecs_component_type(world, &coin_taken_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);
	const game_state *game = game_state_get(world);
	const uint32_t restarts = game != NULL ? game->restarts : 0;
	const bool playing = game != NULL && game->phase == GAME_PHASE_PLAYING;

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more coin rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_ecs_entity entity = entities[i];
		const coin_taken *taken =
			voe_ecs_component_get(world, taken_type, entity);
		bool queued = true;

		if (voe_ecs_component_get(world, transform_type, entity) == NULL)
			continue;
		if (taken != NULL && taken->restarts != restarts)
			queued = coin_put_back(world, entity, taken);
		else if (taken == NULL && playing &&
			 coin_touched_by_player(world, entity))
			queued = coin_take(world, audio, entity, restarts);
		if (!queued)
			return;
	}
}
