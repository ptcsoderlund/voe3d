// The follow camera system: an entity kept behind its target. Teaches
// following an ENTITY field safely, since the target may have died since it
// was authored, and a small piece of maths a project writes for itself when
// math/ has none.
//
// Constraints: the follower's rotation is kept, never turned toward the
// target; a full transform queue drops the rest of this frame's follows.
#include "follow_camera.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <scene/transform_system.h>

const struct voe_ecs_key follow_camera_key = { "follow_camera" };

static const follow_camera follow_camera_default = { .distance = 6.0f };

bool follow_camera_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering follow camera in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&follow_camera_key, sizeof(follow_camera),
		VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(follow_camera),
		&follow_camera_default, "Follow Camera" });
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

void follow_camera_system_run(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "following in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &follow_camera_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const follow_camera *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more follow camera rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *own = voe_ecs_component_get(
			world, transform_type, entities[i]);
		const voe_scene_transform *target = voe_ecs_component_get(
			world, transform_type, rows[i].target);

		if (own == NULL || target == NULL)
			continue;
		voe_scene_transform moved = *own;

		moved.position = voe_math_float3_sub(
			target->position,
			voe_math_float3_scale(forward_of(own->rotation),
					      rows[i].distance));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], moved }))
			return;
	}
}
