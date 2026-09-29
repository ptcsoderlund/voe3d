// The tank gun system: fire held into spawned shots. Teaches spawning a
// cooked prefab from game code (0283 point 10): the shot is made at the
// gun's world muzzle, turned as the gun is, and lands at the step's
// structural apply.
//
// The gun's world transform is read, the muzzle offset is turned by its
// rotation and added to its position, and the prefab's root is spawned there
// with that rotation, so a shell flies along the gun's -Z. The gun's own
// `wait` row is the only thing written here, whole, through
// voe_ecs_component_set.
//
// Constraints: the left mouse button or Space, level not edge; at most one
// shot a gun a step, so a rate above the step rate fires at the step rate.
// Nothing with no window (headless). A gun with no transform, a rate at or
// below zero or a prefab name with no NUL does not fire. A refused spawn
// leaves `wait` as it was, so the gun tries again next step.
#include "tank_gun.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/world.h>

#include <math/double3.h>
#include <math/quat.h>

#include <platform/input.h>

#include <scene/transform_component.h>

#include <math.h>
#include <string.h>

const struct voe_ecs_key tank_gun_key = { "tank_gun" };

static const tank_gun tank_gun_default = {
	.prefab = "shell",
	.rate = 6.0f,
	.muzzle = { 0.0f, 0.3f, -1.2f },
	.wait = 0.0f,
};

bool tank_gun_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank gun in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_gun_key, sizeof(tank_gun), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_gun), &tank_gun_default,
		"Tank / Gun" });
}

// v turned by the unit quaternion q: v + 2w(u x v) + 2u x (u x v).
// math/quat.h has no rotate, and this is all of it needed.
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

// Spawns the gun's prefab at its world muzzle. False when the spawn is
// refused or the gun cannot fire.
static bool fire(const voe_game_project_step *step, voe_ecs_entity entity,
		 const tank_gun *gun)
{
	VOE_BASE_ASSERT(step != NULL && gun != NULL, "firing no gun");
	if (voe_scene_transform_get(step->world, entity) == NULL ||
	    memchr(gun->prefab, '\0', sizeof(gun->prefab)) == NULL)
		return false;
	const voe_scene_transform placed =
		voe_scene_transform_world(step->world, entity);
	const voe_math_double3 muzzle = voe_math_double3_add(
		placed.position,
		voe_math_double3_from_float3(
			turned_by(placed.rotation, gun->muzzle)));
	voe_ecs_entity root;

	return voe_game_project_spawn(step, gun->prefab, muzzle,
				      placed.rotation, &root);
}

void tank_gun_system_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"firing guns in no world");
	VOE_BASE_ASSERT(step->seconds >= 0.0, "firing guns back in time");
	if (step->window == NULL)
		return;
	const bool held =
		voe_platform_input_button_down(step->window,
					       VOE_PLATFORM_BUTTON_LEFT) ||
		voe_platform_input_key_down(step->window, VOE_PLATFORM_KEY_SPACE);
	const voe_ecs_type type =
		voe_ecs_component_type(step->world, &tank_gun_key);
	const tank_gun *rows = voe_ecs_component_rows(step->world, type);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(step->world, type);
	const uint32_t count = voe_ecs_component_count(step->world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank gun rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		tank_gun next = rows[i];

		next.wait -= (float)step->seconds;
		if (held && next.wait <= 0.0f && next.rate > 0.0f &&
		    fire(step, entities[i], &next))
			next.wait = 1.0f / next.rate;
		const bool ok = voe_ecs_component_set(step->world, type,
						      entities[i], &next);

		VOE_BASE_ASSERT(ok, "a tank gun row vanished while stepping it");
	}
}
