// The rotator system: turns each rotator's transform about the world's Y by
// its degrees a second, converted to radians because every engine angle is.
// The turn is a transform intent, applied in the world step after the
// systems, so the system writes no transform row itself.
//
// Constraints: a rotator without a transform is skipped; a full transform
// queue drops the rest of this step's turns.
#include "rotator.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <math/quat.h>

#include <scene/transform_system.h>

#define ROTATOR_RADIANS_PER_DEGREE (3.14159265358979323846 / 180.0)

const struct voe_ecs_key rotator_key = { "rotator" };

static const rotator rotator_default = { .degrees_per_second = 90.0f };

bool rotator_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering rotator in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&rotator_key, sizeof(rotator), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(rotator), &rotator_default,
		"Rotator" });
}

void rotator_system_run(voe_ecs_world *world, double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "turning rotators in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "turning rotators back in time");
	const voe_ecs_type type = voe_ecs_component_type(world, &rotator_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const rotator *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more rotator rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *transform = voe_ecs_component_get(
			world, transform_type, entities[i]);

		if (transform == NULL)
			continue;
		const float radians = (float)(rows[i].degrees_per_second *
					      ROTATOR_RADIANS_PER_DEGREE *
					      seconds);
		voe_scene_transform turned = *transform;

		turned.rotation = voe_math_quat_normalize(voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f }, radians),
			transform->rotation));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], turned }))
			return;
	}
}
