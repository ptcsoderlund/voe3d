// The tank shell system: a spawned shot's flight and end. Teaches removing
// what game code spawned (0283 point 10): at the end of its life a shell is
// queued for removal with its tree, and goes at the step's structural apply.
//
// A shell is a spawned root, so its transform row is its world one; it
// moves along its own -Z through one transform intent a step, as the hull
// drives. Its `life` row is written here, whole, through
// voe_ecs_component_set; a shell whose life is spent is not moved again.
//
// tank_shell_fire spawns a shot and queues its tank_shot row (tank_shell.h)
// behind the spawn's rows on the structural queue.
//
// Constraints: runs headless as well, nothing here reads input. A full
// transform queue leaves the rest of the shells where they were this step;
// a full structural queue leaves a spent shell to be removed next step. The
// runtime-only marker is taken by address at run time, because a project
// library imports it on Windows (0245).
#include "tank_shell.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <scene/transform_system.h>

const struct voe_ecs_key tank_shell_key = { "tank_shell" };
const struct voe_ecs_key tank_shot_key = { "tank_shot" };

static const tank_shell tank_shell_default = { .speed = 30.0f, .life = 3.0f };

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
		const voe_scene_transform *transform =
			voe_scene_transform_get(step->world, entities[i]);

		if (transform == NULL || queue_full)
			continue;
		voe_scene_transform flown = *transform;

		flown.position = voe_math_double3_add(
			transform->position,
			voe_math_double3_from_float3(voe_math_float3_scale(
				forward_of(transform->rotation),
				next.speed * (float)step->seconds)));
		queue_full = !voe_scene_transform_submit(
			step->world,
			(voe_scene_transform_intent){ entities[i], flown });
	}
}
