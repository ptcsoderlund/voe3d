// The tank hull system: WASD into a hull's turn and drive. Teaches driving a
// thing by its own facing: the turn is about the world's up, and the drive
// is along the hull's own -Z after that turn, so W is always forward.
//
// A hull is a root and moves only its own row: the turret and the barrel
// are its children and ride on it by parenting (0271), so nothing here
// touches them. Each hull gets one transform intent a step, turn and drive
// together.
//
// Constraints: W/A/S/D only, by place on the keyboard; opposite keys cancel.
// Nothing with no window (headless). A full transform queue leaves the rest
// of the hulls where they were this step.
#include "tank_hull.h"

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>

#include <platform/input.h>

#include <scene/transform_system.h>

#define TANK_HULL_RADIANS_PER_DEGREE (3.14159265358979323846f / 180.0f)

const struct voe_ecs_key tank_hull_key = { "tank_hull" };

static const tank_hull tank_hull_default = { .speed = 4.0f, .turn = 90.0f };

bool tank_hull_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank hull in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_hull_key, sizeof(tank_hull), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_hull), &tank_hull_default,
		"Tank / Hull" });
}

// -1, 0 or 1 from a pair of opposite keys.
static float key_axis(voe_platform_window *window, voe_platform_key minus,
		      voe_platform_key plus)
{
	return (voe_platform_input_key_down(window, plus) ? 1.0f : 0.0f) -
	       (voe_platform_input_key_down(window, minus) ? 1.0f : 0.0f);
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

void tank_hull_system_run(voe_ecs_world *world, voe_platform_window *window,
			  double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "driving hulls in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "driving hulls back in time");
	if (window == NULL)
		return;
	const float drive = key_axis(window, VOE_PLATFORM_KEY_S,
				     VOE_PLATFORM_KEY_W);
	const float turn = key_axis(window, VOE_PLATFORM_KEY_D,
				    VOE_PLATFORM_KEY_A);
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_hull_key);
	const voe_ecs_type transform_type =
		voe_ecs_component_type(world, &voe_scene_transform_key);
	const tank_hull *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank hull rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_transform *transform = voe_ecs_component_get(
			world, transform_type, entities[i]);

		if (transform == NULL)
			continue;
		voe_scene_transform driven = *transform;

		driven.rotation = voe_math_quat_normalize(voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f },
				turn * rows[i].turn *
					TANK_HULL_RADIANS_PER_DEGREE *
					(float)seconds),
			transform->rotation));
		driven.position = voe_math_double3_add(
			transform->position,
			voe_math_double3_from_float3(voe_math_float3_scale(
				forward_of(driven.rotation),
				drive * rows[i].speed * (float)seconds)));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i], driven }))
			return;
	}
}
