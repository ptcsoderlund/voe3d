// The tank enemy system: a spawned enemy's drive, fire and end. As the shell
// system does (0283 point 10), an enemy whose life is spent is queued for
// removal with its tree, its turret with it, and goes at the step's
// structural apply.
//
// An enemy is a spawned root, so its transform row is its world one; it
// drives along its own -Z through one transform intent a step. Its row, life
// and wait, is written here, whole, once a step, through
// voe_ecs_component_set; an enemy whose life is spent neither fires nor moves
// again.
//
// The aim (0297): the target is the tank_lives row's entity at its world
// position. An enemy's turret is the first tank_turret row whose parent row
// names the enemy; an enemy with none never fires. With the target within
// `range` of the enemy, the turret turns toward it by tank_turret_turn_toward
// for the step's seconds, which gives the barrel and the degrees left.
//
// The fire rule (0294 point 5, amended by 0297): `wait` counts down every
// step; the enemy fires only when it is spent, the turn was not refused and
// at most TANK_ENEMY_ON_TARGET degrees are left. The muzzle is turned by the
// barrel and added to the turret's world position, and the shell spawns there
// turned as the barrel through tank_shell_fire: `owner` the enemy, `from` its
// position. `wait` becomes 1 / `rate` only when the fire is not refused, so a
// refused one tries again next step. Each shot bursts the turret's emitter,
// the prefab's muzzle flash, and restarts its tank_light_fade, which lights
// the flash's light (0322 point 6); a turret with neither sends nothing. Each
// shot also
// plays `Assets/sounds/shot.wav` at the muzzle through the step's mixer.
//
// The hum (0304 point 8): an enemy with no sound gets the hull's engine,
// looping, quieter and lower. Its removal takes the sound with it (0304
// point 4), so nothing here stops it.
//
// Constraints: runs headless as well, nothing here reads input. At most one
// shot an enemy a step. Finding a turret scans every turret row, enemies
// times turrets a step; a turret field on the enemy row would lift it. A full
// transform queue leaves the rest of the enemies where they were this step;
// a full structural queue leaves a spent enemy to be removed next step.
#include "tank_enemy.h"
#include "tank_light_fade.h"
#include "tank_lives.h"
#include "tank_shell.h"
#include "tank_turret.h"

#include <3d/emitter_component.h>

#include <audio/sound_component.h>

#include <base/assert.h>

#include <ecs/component.h>

#include <game/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <scene/parent_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <string.h>

// The most degrees between the barrel and the way to the target that an
// enemy still fires at.
#define TANK_ENEMY_ON_TARGET 2.0f

const struct voe_ecs_key tank_enemy_key = { "tank_enemy" };

static const tank_enemy tank_enemy_default = {
	.speed = 2.0f,
	.life = 20.0f,
	.prefab = "shell",
	.rate = 0.5f,
	.range = 30.0f,
	.muzzle = { 0.0f, 0.5f, -3.0f },
	.wait = 0.0f,
};

// Whom the enemies fire at this step: the player's hull and where it is.
typedef struct {
	bool found;
	voe_ecs_entity entity;
	voe_math_double3 position;
} tank_enemy_target;

bool tank_enemy_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank enemy in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_enemy_key, sizeof(tank_enemy), TANK_ENEMY_ROWS,
		VOE_GAME_PROJECT_DESCRIPTION(tank_enemy), &tank_enemy_default,
		"Tank / Enemy" }) &&
	       voe_game_project_component_needs(world, &tank_enemy_key,
						&voe_scene_transform_key);
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

// v turned by the unit quaternion q: v + 2w(u x v) + 2u x (u x v).
static voe_math_float3 turned_by(voe_math_quat q, voe_math_float3 v)
{
	const voe_math_float3 u = { q.x, q.y, q.z };
	const voe_math_float3 t =
		voe_math_float3_scale(voe_math_float3_cross(u, v), 2.0f);
	const voe_math_float3 out = voe_math_float3_add(
		voe_math_float3_add(v, voe_math_float3_scale(t, q.w)),
		voe_math_float3_cross(u, t));

	VOE_BASE_DEBUG_ASSERT(isfinite(out.x) && isfinite(out.z),
			      "a muzzle turned to nowhere");
	return out;
}

// The lives row's entity and its world position; not found with no row or
// no transform on it.
static tank_enemy_target target_of(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "finding a target in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_lives_key);
	tank_enemy_target target = { 0 };

	if (voe_ecs_component_count(world, type) == 0)
		return target;
	target.entity = voe_ecs_component_entities(world, type)[0];
	if (voe_scene_transform_get(world, target.entity) == NULL)
		return target;
	target.position = voe_scene_transform_world(world, target.entity).position;
	target.found = true;
	VOE_BASE_DEBUG_ASSERT(isfinite(target.position.x), "a target nowhere");
	return target;
}

// The enemy's turret: the first tank_turret row whose parent row names the
// enemy. False, `turret` untouched, with none.
static bool turret_of(const voe_ecs_world *world, voe_ecs_entity enemy,
		      voe_ecs_entity *turret)
{
	VOE_BASE_ASSERT(world != NULL && turret != NULL, "finding no turret");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_turret_key);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_MAX_DRAWN,
			"more tank turret rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_parent *parent =
			voe_scene_parent_get(world, entities[i]);

		if (parent != NULL && parent->parent.index == enemy.index &&
		    parent->parent.generation == enemy.generation) {
			*turret = entities[i];
			return true;
		}
	}
	return false;
}

// Turns the enemy's turret toward the target within range and, when `ready`
// and on target, fires the enemy's prefab along the barrel. False when it
// did not fire: no target or turret, out of range, a refused turn, not ready,
// off target, or a refused fire.
static bool aim_and_fire(const voe_game_project_step *step,
			 voe_ecs_entity entity, const tank_enemy *enemy,
			 const tank_enemy_target *target, bool ready)
{
	VOE_BASE_ASSERT(step != NULL && enemy != NULL && target != NULL,
			"aiming no enemy");
	const voe_scene_transform *transform =
		voe_scene_transform_get(step->world, entity);
	voe_ecs_entity turret = { 0 };

	if (!target->found || transform == NULL ||
	    !turret_of(step->world, entity, &turret))
		return false;
	const voe_math_float3 way = voe_math_double3_to_float3(
		voe_math_double3_sub(target->position, transform->position));

	if (voe_math_float3_length(way) > enemy->range)
		return false;
	voe_math_quat barrel = { 0 };
	float left = 0.0f;

	if (!tank_turret_turn_toward(step->world, turret, target->position,
				     step->seconds, &barrel, &left) ||
	    !ready || left > TANK_ENEMY_ON_TARGET ||
	    memchr(enemy->prefab, '\0', sizeof(enemy->prefab)) == NULL ||
	    voe_scene_transform_get(step->world, turret) == NULL)
		return false;
	const voe_math_double3 muzzle = voe_math_double3_add(
		voe_scene_transform_world(step->world, turret).position,
		voe_math_double3_from_float3(turned_by(barrel, enemy->muzzle)));

	if (!tank_shell_fire(step, enemy->prefab, muzzle, barrel, entity,
			     transform->position))
		return false;
	if (step->audio != NULL)
		(void)voe_audio_mixer_play_at(step->audio, "Assets/sounds/shot.wav",
					      muzzle);
	// The turret's own flash (0299 point 2) and its light's fade; a full
	// queue loses only that flash, and no fade row sends nothing.
	if (voe_3d_emitter_get(step->world, turret) != NULL)
		(void)voe_3d_emitter_control_submit(
			step->world, (voe_3d_emitter_control){
				.entity = turret, .kind = VOE_3D_EMITTER_BURST,
				.count = 0 });
	(void)tank_light_fade_start(step->world, turret);
	return true;
}

// Moves the enemy along its own -Z for the step. False when the transform
// queue is full.
static bool drive(const voe_game_project_step *step, voe_ecs_entity entity,
		  float speed)
{
	VOE_BASE_ASSERT(step != NULL, "driving no enemy");
	const voe_scene_transform *transform =
		voe_scene_transform_get(step->world, entity);

	if (transform == NULL)
		return true;
	voe_scene_transform driven = *transform;

	driven.position = voe_math_double3_add(
		transform->position,
		voe_math_double3_from_float3(voe_math_float3_scale(
			forward_of(transform->rotation),
			speed * (float)step->seconds)));
	return voe_scene_transform_submit(
		step->world, (voe_scene_transform_intent){ entity, driven });
}

// Gives an enemy with no sound the engine hum, looping, at volume 0.35 and
// pitch 0.85. A refused add is left for the next step.
static void hum(voe_ecs_world *world, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "humming in no world");
	if (voe_audio_sound_get(world, entity) != NULL)
		return;
	const voe_audio_sound engine = {
		.path = "Assets/sounds/engine.wav", .playing = true, .loop = true,
		.volume = 0.35f, .pitch = 0.85f,
	};

	VOE_BASE_DEBUG_ASSERT(engine.volume > 0.0f, "a hum never heard");
	(void)voe_audio_sound_add(world, entity, engine);
}

void tank_enemy_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"driving enemies in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "driving enemies back in time");
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_enemy_key);
	const tank_enemy *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);
	const tank_enemy_target target = target_of(step->world);
	bool queue_full = false;

	VOE_BASE_ASSERT(count <= TANK_ENEMY_ROWS,
			"more tank enemy rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_enemy next = rows[i];

		next.life -= (float)step->seconds;
		if (next.life > 0.0f) {
			next.wait -= (float)step->seconds;
			if (aim_and_fire(step, entities[i], &next, &target,
					 next.wait <= 0.0f && next.rate > 0.0f))
				next.wait = 1.0f / next.rate;
		}
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank enemy row vanished while stepping it");
		if (next.life <= 0.0f) {
			// Refused: the queue is full, and next step tries again.
			(void)voe_game_project_remove(step, entities[i]);
			continue;
		}
		hum(step->world, entities[i]);
		if (!queue_full)
			queue_full = !drive(step, entities[i], next.speed);
	}
}
