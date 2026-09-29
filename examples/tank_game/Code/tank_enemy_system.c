// The tank enemy system: a spawned enemy's drive and end. As the shell
// system does (0283 point 10), an enemy whose life is spent is queued for
// removal with its tree, its turret with it, and goes at the step's
// structural apply.
//
// An enemy is a spawned root, so its transform row is its world one; it
// drives along its own -Z through one transform intent a step. Its `life`
// row is written here, whole, through voe_ecs_component_set; an enemy whose
// life is spent is not moved again.
//
// Constraints: runs headless as well, nothing here reads input. A full
// transform queue leaves the rest of the enemies where they were this step;
// a full structural queue leaves a spent enemy to be removed next step.
#include "tank_enemy.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <scene/transform_system.h>

const struct voe_ecs_key tank_enemy_key = { "tank_enemy" };

static const tank_enemy tank_enemy_default = { .speed = 2.0f, .life = 20.0f };

bool tank_enemy_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank enemy in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_enemy_key, sizeof(tank_enemy), TANK_ENEMY_ROWS,
		VOE_GAME_PROJECT_DESCRIPTION(tank_enemy), &tank_enemy_default,
		"Tank / Enemy" });
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
	bool queue_full = false;

	VOE_BASE_ASSERT(count <= TANK_ENEMY_ROWS,
			"more tank enemy rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_enemy next = rows[i];

		next.life -= (float)step->seconds;
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank enemy row vanished while stepping it");
		if (next.life <= 0.0f) {
			// Refused: the queue is full, and next step tries again.
			(void)voe_game_project_remove(step, entities[i]);
			continue;
		}
		const voe_scene_transform *transform =
			voe_scene_transform_get(step->world, entities[i]);

		if (transform == NULL || queue_full)
			continue;
		voe_scene_transform driven = *transform;

		driven.position = voe_math_double3_add(
			transform->position,
			voe_math_double3_from_float3(voe_math_float3_scale(
				forward_of(transform->rotation),
				next.speed * (float)step->seconds)));
		queue_full = !voe_scene_transform_submit(
			step->world,
			(voe_scene_transform_intent){ entities[i], driven });
	}
}
