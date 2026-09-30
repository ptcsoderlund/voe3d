// The tank spawner system: enemies made from a cooked prefab on a timer
// (0283 point 10). The spawn is made at the spawner's world position, turned
// as the spawner is, and lands at the step's structural apply.
//
// The enemies are counted once, as `tank_enemy` rows, and each spawn this
// step adds one to that count, since what is queued is not a row yet. The
// spawner's own `wait` row is the only thing written here, whole, through
// voe_ecs_component_set.
//
// Constraints: at most one spawn a spawner a step. Runs headless as well,
// nothing here reads input. A spawner with no transform or a prefab name with
// no NUL does not spawn. A refused spawn leaves `wait` as it was, so the
// spawner tries again next step.
#include "tank_spawner.h"

#include "tank_enemy.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/world.h>

#include <scene/transform_component.h>

#include <string.h>

const struct voe_ecs_key tank_spawner_key = { "tank_spawner" };

static const tank_spawner tank_spawner_default = {
	.prefab = "enemy_tank",
	.every = 4.0f,
	.most = 6,
	.wait = 0.0f,
};

bool tank_spawner_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank spawner in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_spawner_key, sizeof(tank_spawner), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_spawner),
		&tank_spawner_default, "Tank / Spawner",
		&voe_scene_transform_key });
}

// Spawns the spawner's prefab at its world transform. False when the spawn
// is refused or the spawner cannot spawn.
static bool spawn(const voe_game_project_step *step, voe_ecs_entity entity,
		  const tank_spawner *spawner)
{
	VOE_BASE_ASSERT(step != NULL && spawner != NULL, "spawning from nothing");
	if (voe_scene_transform_get(step->world, entity) == NULL ||
	    memchr(spawner->prefab, '\0', sizeof(spawner->prefab)) == NULL)
		return false;
	const voe_scene_transform placed =
		voe_scene_transform_world(step->world, entity);
	voe_ecs_entity root;

	return voe_game_project_spawn(step, spawner->prefab, placed.position,
				      placed.rotation, &root);
}

void tank_spawner_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"spawning enemies in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "spawning enemies back in time");
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_spawner_key);
	const tank_spawner *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);
	uint32_t enemies = voe_ecs_component_count(
		step->world, voe_ecs_component_type(step->world, &tank_enemy_key));

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank spawner rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_spawner next = rows[i];

		next.wait -= (float)step->seconds;
		if (enemies < next.most && next.wait <= 0.0f &&
		    spawn(step, entities[i], &next)) {
			next.wait = next.every;
			enemies++;
		}
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank spawner row vanished while stepping it");
	}
}
