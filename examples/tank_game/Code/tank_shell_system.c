// The tank shell system: a spawned shot's flight, hit and end. Teaches
// removing what game code spawned (0283 point 10): at the end of its life or
// on a hit a shell is queued for removal with its tree, and goes at the
// step's structural apply.
//
// A shell is a spawned root, so its transform row is its world one; it
// moves along its own -Z through one transform intent a step, as the hull
// drives. Its `life` row is written here, whole, through
// voe_ecs_component_set; a shell whose life is spent is not moved again.
//
// THE SWEEP (0293, 0294 point 2): the solid colliders are gathered once a
// step into a local array; each shell with life left sweeps its radius from
// its shot's `from` (its position with no shot row) to where it would fly,
// past its shot's `owner` (itself with no row). A hit writes the shot row's
// `hit` and `target` and queues the shell's removal, unmoved; no hit moves
// it and its `from` becomes where it went. Shot rows are written whole.
//
// THE SWAP (0294 point 3, 0296): only the player's shots wreck. The player's
// hull, the tank_lives row's entity, is found once a step; a hit swaps only
// when the shell has a shot row and that hull is within its `owner`. Any
// other hit stops the shell and changes nothing. Then a hit target with a
// tank_breakable row, not yet swapped this step, has its `wreck` spawned at
// its world place and, only when that is not refused, is removed with its
// tree. Swapped targets are a local list, so many shells hitting one thing
// swap it once.
//
// tank_shell_fire spawns a shot and queues its tank_shot row (tank_shell.h)
// behind the spawn's rows on the structural queue.
//
// Constraints: runs headless as well, nothing here reads input. A full
// transform queue leaves the rest of the shells where they were this step;
// a full structural queue leaves a spent or hit shell to be removed next
// step. Colliders past VOE_GAME_WORLD_MAX_DRAWN are not hit. A radius that
// is negative or not finite sweeps as 0; a flight that is not finite is not
// swept or moved. The runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245).
#include "tank_shell.h"

#include "tank_breakable.h"
#include "tank_lives.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <physics/sweep.h>

#include <scene/parent_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <string.h>

const struct voe_ecs_key tank_shell_key = { "tank_shell" };
const struct voe_ecs_key tank_shot_key = { "tank_shot" };

static const tank_shell tank_shell_default = {
	.speed = 30.0f, .life = 3.0f, .radius = 0.1f
};

// The targets swapped for their wrecks this step; one a shell at most.
struct swapped {
	voe_ecs_entity entities[TANK_SHELL_ROWS];
	uint32_t count;
};

bool tank_shell_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank shell in no world");
	const bool shell = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_shell_key, sizeof(tank_shell), TANK_SHELL_ROWS,
			VOE_GAME_PROJECT_DESCRIPTION(tank_shell),
			&tank_shell_default, "Tank / Shell" });
	const bool shot = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_shot_key, sizeof(tank_shot), TANK_SHELL_ROWS,
			&voe_ecs_runtime_only, NULL, NULL });

	return shell && shot;
}

bool tank_shell_fire(const voe_game_project_step *step, const char *prefab,
		     voe_math_double3 position, voe_math_quat rotation,
		     voe_ecs_entity owner, voe_math_double3 from)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"firing a shell in no world");
	VOE_BASE_ASSERT(prefab != NULL, "firing no prefab");
	const tank_shot shot = { .owner = owner, .from = from };
	voe_ecs_entity root;

	if (!voe_game_project_spawn(step, prefab, position, rotation, &root))
		return false;
	return voe_ecs_structure_add(
		step->world, voe_ecs_component_type(step->world, &tank_shot_key),
		root, &shot);
}

// (0, 0, -1) rotated by the unit quaternion q: the third column of its
// matrix, negated. math/quat.h has no rotate, and this is all of it needed.
static voe_math_float3 forward_of(voe_math_quat q)
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

static bool swapped_has(const struct swapped *list, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(list != NULL, "searching no swapped list");
	VOE_BASE_DEBUG_ASSERT(list->count <= TANK_SHELL_ROWS,
			      "a swapped list past its room");
	for (uint32_t i = 0; i < list->count; i++)
		if (list->entities[i].index == entity.index &&
		    list->entities[i].generation == entity.generation)
			return true;
	return false;
}

// Spawns a breakable target's wreck at its world place and, when that is
// not refused, removes the target. A target already swapped this step, not
// breakable, with no transform or with an empty or unterminated wreck name
// is left alone.
static void swap_for_wreck(const voe_game_project_step *step,
			   voe_ecs_entity target, struct swapped *list)
{
	VOE_BASE_ASSERT(step != NULL && list != NULL, "swapping in nothing");
	const tank_breakable *breakable = voe_ecs_component_get(
		step->world,
		voe_ecs_component_type(step->world, &tank_breakable_key),
		target);

	if (breakable == NULL || swapped_has(list, target) ||
	    voe_scene_transform_get(step->world, target) == NULL ||
	    breakable->wreck[0] == '\0' ||
	    memchr(breakable->wreck, '\0', sizeof(breakable->wreck)) == NULL)
		return;
	const voe_scene_transform placed =
		voe_scene_transform_world(step->world, target);
	voe_ecs_entity wreck;

	if (!voe_game_project_spawn(step, breakable->wreck, placed.position,
				    placed.rotation, &wreck))
		return;
	// Refused: the queue is full, and the target stays until hit again.
	(void)voe_game_project_remove(step, target);
	VOE_BASE_ASSERT(list->count < TANK_SHELL_ROWS,
			"more swaps than shells this step");
	list->entities[list->count++] = target;
}

// Writes the shot row whole, when the shell has one.
static void shot_write(voe_ecs_world *world, voe_ecs_entity entity,
		       bool has_row, const tank_shot *shot)
{
	VOE_BASE_ASSERT(world != NULL && shot != NULL, "writing no shot");
	if (!has_row)
		return;
	const bool ok = voe_ecs_component_set(
		world, voe_ecs_component_type(world, &tank_shot_key), entity,
		shot);

	VOE_BASE_ASSERT(ok, "a shot row vanished while stepping it");
}

// Sweeps one live shell and stops it on a hit or moves it. False when the
// transform queue refused the move. `player` is the player's hull, NULL when
// there is none; only a shot whose owner holds it swaps (0296).
static bool fly(const voe_game_project_step *step, voe_ecs_entity entity,
		const tank_shell *shell, const voe_physics_obstacle *obstacles,
		uint32_t count, const voe_ecs_entity *player,
		struct swapped *list)
{
	VOE_BASE_ASSERT(step != NULL && shell != NULL && list != NULL,
			"flying no shell");
	const voe_scene_transform *transform =
		voe_scene_transform_get(step->world, entity);

	VOE_BASE_ASSERT(transform != NULL, "flying a shell with no transform");
	const tank_shot *row = voe_ecs_component_get(
		step->world, voe_ecs_component_type(step->world, &tank_shot_key),
		entity);
	tank_shot shot = row != NULL ? *row : (tank_shot){
		.owner = entity, .from = transform->position };
	const voe_math_double3 to = voe_math_double3_add(
		transform->position,
		voe_math_double3_from_float3(voe_math_float3_scale(
			forward_of(transform->rotation),
			shell->speed * (float)step->seconds)));
	const voe_math_float3 motion =
		voe_math_double3_to_float3(voe_math_double3_sub(to, shot.from));
	const float radius = isfinite(shell->radius) && shell->radius > 0.0f ?
		shell->radius : 0.0f;
	voe_physics_hit hit;

	if (!isfinite(motion.x) || !isfinite(motion.y) || !isfinite(motion.z))
		return true;
	if (voe_physics_sweep(obstacles, count, shot.from, motion, radius,
			      shot.owner, &hit)) {
		shot.hit = true;
		shot.target = hit.entity;
		shot_write(step->world, entity, row != NULL, &shot);
		// Refused: the queue is full, and next step tries again.
		(void)voe_game_project_remove(step, entity);
		if (row != NULL && player != NULL &&
		    voe_scene_parent_within(step->world, *player, shot.owner))
			swap_for_wreck(step, hit.entity, list);
		return true;
	}
	voe_scene_transform flown = *transform;

	flown.position = to;
	if (!voe_scene_transform_submit(
		    step->world, (voe_scene_transform_intent){ entity, flown }))
		return false;
	shot.from = to;
	shot_write(step->world, entity, row != NULL, &shot);
	return true;
}

void tank_shell_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"flying shells in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "flying shells back in time");
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_shell_key);
	const tank_shell *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);
	voe_physics_obstacle obstacles[VOE_GAME_WORLD_MAX_DRAWN];
	const uint32_t obstacle_count = voe_physics_obstacles_gather(
		step->world, obstacles, VOE_GAME_WORLD_MAX_DRAWN);
	struct swapped list = { .count = 0 };
	const voe_ecs_type lives =
		voe_ecs_component_type(step->world, &tank_lives_key);
	const voe_ecs_entity *player =
		voe_ecs_component_count(step->world, lives) > 0 ?
		&voe_ecs_component_entities(step->world, lives)[0] : NULL;
	bool queue_full = false;

	VOE_BASE_ASSERT(count <= TANK_SHELL_ROWS,
			"more tank shell rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_shell next = rows[i];

		next.life -= (float)step->seconds;
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank shell row vanished while stepping it");
		if (next.life <= 0.0f) {
			// Refused: the queue is full, and next step tries again.
			(void)voe_game_project_remove(step, entities[i]);
			continue;
		}
		if (voe_scene_transform_get(step->world, entities[i]) == NULL ||
		    queue_full)
			continue;
		queue_full = !fly(step, entities[i], &next, obstacles,
				  obstacle_count, player, &list);
	}
}
