// A light blocker's box in the world — see 3d/light_blocker.h.
//
// The row and the transform are both looked up before anything is computed,
// so a missing one leaves *out as it was; the place is
// voe_scene_transform_between at the lag, so a parent carries the box.
#include <3d/light_blocker.h>
#include <base/assert.h>
#include <physics/collider_component.h>
#include <scene/light_blocker_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stddef.h>

bool voe_3d_light_blocker_shape(const voe_ecs_world *world,
				voe_ecs_entity entity, float lag,
				voe_physics_shape *out)
{
	VOE_BASE_ASSERT(world != NULL, "a light blocker of no world");
	VOE_BASE_ASSERT(out != NULL, "a light blocker into nothing");

	const voe_scene_light_blocker *row =
		voe_scene_light_blocker_get(world, entity);

	if (row == NULL || voe_scene_transform_get(world, entity) == NULL)
		return false;
	voe_scene_transform place =
		voe_scene_transform_between(world, entity, lag);

	*out = (voe_physics_shape){
		.kind = VOE_PHYSICS_COLLIDER_BOX,
		.centre = place.position,
		.rotation = place.rotation,
		.half = { fabsf(place.scale.x) * row->size.x * 0.5f,
			  fabsf(place.scale.y) * row->size.y * 0.5f,
			  fabsf(place.scale.z) * row->size.z * 0.5f },
	};
	return true;
}
