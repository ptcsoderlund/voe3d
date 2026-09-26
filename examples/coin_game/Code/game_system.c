// The game module: registers game_state, reads the score out of the player
// and the coins, and steps the run's clock and its end.
//
// Constraints: the only writer of game_state rows. The runtime-only marker is
// taken by address at run time, because a project library imports it on
// Windows (0245).
#include "coin.h"
#include "game_state.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>
#include <game/world.h>

#include <math.h>

const struct voe_ecs_key game_state_key = { "game_state" };

bool game_state_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering game_state in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&game_state_key, sizeof(game_state), 1,
		&voe_ecs_runtime_only, NULL, NULL });
}

const game_state *game_state_get(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "reading game_state in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &game_state_key);

	if (voe_ecs_component_count(world, type) == 0)
		return NULL;
	return voe_ecs_component_rows(world, type);
}

// Whether the coin was taken under the current restart count.
static bool game_coin_taken(const voe_ecs_world *world, voe_ecs_type taken_type,
			    voe_ecs_entity coin_entity, uint32_t restarts)
{
	const coin_taken *taken =
		voe_ecs_component_get(world, taken_type, coin_entity);

	return taken != NULL && taken->restarts == restarts;
}

// Walks the coins once: how many are left, and the points of those taken,
// under `state`'s restart count (0 with no state yet).
static uint32_t game_coins_walk(const voe_ecs_world *world,
				const game_state *state, int64_t *points)
{
	VOE_BASE_ASSERT(world != NULL && points != NULL,
			"walking coins with nowhere to count");
	const uint32_t restarts = state != NULL ? state->restarts : 0;
	const voe_ecs_type type = voe_ecs_component_type(world, &coin_key);
	const voe_ecs_type taken_type =
		voe_ecs_component_type(world, &coin_taken_key);
	const coin *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);
	uint32_t left = 0;

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more coin rows than were registered");
	*points = 0;
	for (uint32_t i = 0; i < count; i++) {
		if (game_coin_taken(world, taken_type, entities[i], restarts))
			*points += rows[i].points;
		else
			left++;
	}
	return left;
}

// The first player's row, NULL with no player.
static const player *game_player(const voe_ecs_world *world)
{
	const voe_ecs_type type = voe_ecs_component_type(world, &player_key);

	if (voe_ecs_component_count(world, type) == 0)
		return NULL;
	return voe_ecs_component_rows(world, type);
}

// The score `state` stands for, never below 0.
static int32_t game_score_of(const voe_ecs_world *world,
			     const game_state *state)
{
	VOE_BASE_ASSERT(world != NULL, "scoring in no world");
	const player *first = game_player(world);
	int64_t points = 0;

	(void)game_coins_walk(world, state, &points);
	int64_t score = points - (state != NULL ? state->dropped : 0);

	if (first != NULL)
		score += first->start_score;
	score = score < 0 ? 0 : score > INT32_MAX ? INT32_MAX : score;
	VOE_BASE_ASSERT(score >= 0, "a score below zero");
	return (int32_t)score;
}

int32_t game_score(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "scoring in no world");
	return game_score_of(world, game_state_get(world));
}

uint32_t game_coins_left(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "counting coins in no world");
	int64_t points = 0;

	return game_coins_walk(world, game_state_get(world), &points);
}

// Queues the row, phase menu, on the first player; nothing without a player.
// A second add before the queue applies is dropped by the queue.
static void game_state_make(voe_ecs_world *world, voe_ecs_type type)
{
	const voe_ecs_type player_type =
		voe_ecs_component_type(world, &player_key);

	VOE_BASE_ASSERT(game_state_get(world) == NULL,
			"making game_state a second time");
	if (voe_ecs_component_count(world, player_type) == 0)
		return;
	const game_state fresh = { .phase = GAME_PHASE_MENU };

	// A full queue this step is retried on the next.
	(void)voe_ecs_structure_add(
		world, type,
		voe_ecs_component_entities(world, player_type)[0], &fresh);
}

void game_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "stepping the game in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "stepping the game back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &game_state_key);
	const game_state *current = game_state_get(world);

	if (current == NULL) {
		game_state_make(world, type);
		return;
	}
	if (current->phase != GAME_PHASE_PLAYING)
		return;
	const player *first = game_player(world);
	const int64_t drop = first != NULL ? first->score_drop : 0;
	game_state next = *current;
	const double whole = floor(next.banked + seconds);
	const int64_t dropped = next.dropped + (int64_t)whole * drop;
	int64_t points = 0;

	next.banked = next.banked + seconds - whole;
	next.dropped = dropped > INT32_MAX ? INT32_MAX :
		       dropped < INT32_MIN ? INT32_MIN : (int32_t)dropped;
	const uint32_t coins = voe_ecs_component_count(
		world, voe_ecs_component_type(world, &coin_key));
	const uint32_t left = game_coins_walk(world, &next, &points);

	if (game_score_of(world, &next) == 0)
		next.phase = GAME_PHASE_LOST;
	else if (coins > 0 && left == 0)
		next.phase = GAME_PHASE_WON;
	const bool ok = voe_ecs_component_set(
		world, type, voe_ecs_component_entities(world, type)[0], &next);

	VOE_BASE_ASSERT(ok, "game_state vanished while stepping it");
}
