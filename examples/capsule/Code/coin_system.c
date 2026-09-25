// The coin system: a spin and a pickup. Teaches what a trigger is for, a
// place that notices what enters it without stopping it, and that noticing
// is a question: the coin overlaps its own shape against the world, and the
// query only reads (0249 rule 1), so asking changes nothing. The spin is a
// transform intent and the pickup a destroy on the structural queue; both
// land in the world step after the systems, so no system sees a coin vanish
// partway through its run.
//
// Constraints: a box collider is not a query shape (physics/overlap.h), so a
// coin with one spins and is never taken; a player past the first
// COIN_CONTACTS contacts is not seen that step; a full queue drops the rest of
// this step's spins or pickups.
#include "coin.h"
#include "player.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>
#include <game/world.h>

#include <math/quat.h>

#include <physics/collider_component.h>
#include <physics/overlap.h>
#include <physics/shape.h>

#include <scene/transform_system.h>

#define COIN_CONTACTS 8u

const struct voe_ecs_key coin_key = { "coin" };

static const coin coin_default = { .spin = 2.0f };

bool coin_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering coin in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&coin_key, sizeof(coin), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(coin), &coin_default, "Coin" });
}

// True when the coin's own shape overlaps an entity with a player row.
static bool touched_by_player(const voe_ecs_world *world, voe_ecs_entity self)
{
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

void coin_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "turning coins in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "turning coins back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &coin_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const coin *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more coin rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *transform = voe_ecs_component_get(
			world, transform_type, entities[i]);

		if (transform == NULL)
			continue;
		if (touched_by_player(world, entities[i]) &&
		    !voe_ecs_structure_destroy(world, entities[i]))
			return;
		voe_scene_transform turned = *transform;

		turned.rotation = voe_math_quat_normalize(voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f },
				rows[i].spin * (float)seconds),
			transform->rotation));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], turned }))
			return;
	}
}
