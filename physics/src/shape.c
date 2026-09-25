// A collider and its transform turned into a shape in the world. Reads both
// tables and writes neither; physics/shape.h says what each kind's half means.
#include <base/assert.h>
#include <ecs/component.h>
#include <physics/collider_component.h>
#include <physics/shape.h>
#include <scene/transform_component.h>

#include <math.h>

bool voe_physics_shape_of(const voe_ecs_world *world, voe_ecs_entity entity,
			  voe_physics_shape *out)
{
	const voe_physics_collider *collider;
	const voe_scene_transform *transform;
	voe_math_float3 scale;
	voe_math_float3 half = { 0.0f, 0.0f, 0.0f };

	VOE_BASE_DEBUG_ASSERT(world != NULL && out != NULL,
			      "a shape out of no world, or into nowhere");

	collider = voe_physics_collider_get(world, entity);
	transform = voe_scene_transform_get(world, entity);
	if (collider == NULL || transform == NULL)
		return false;

	scale = (voe_math_float3){ fabsf(transform->scale.x),
				   fabsf(transform->scale.y),
				   fabsf(transform->scale.z) };

	switch (collider->kind) {
	case VOE_PHYSICS_COLLIDER_BOX:
		half = (voe_math_float3){ 0.5f * collider->size.x * scale.x,
					  0.5f * collider->size.y * scale.y,
					  0.5f * collider->size.z * scale.z };
		break;
	case VOE_PHYSICS_COLLIDER_SPHERE:
		half.x = 0.5f * collider->size.x *
			 fmaxf(scale.x, fmaxf(scale.y, scale.z));
		break;
	case VOE_PHYSICS_COLLIDER_CAPSULE:
		half.x = 0.5f * collider->size.x * fmaxf(scale.x, scale.z);
		half.y = fmaxf(0.5f * collider->size.y * scale.y, half.x);
		break;
	default:
		return false;
	}

	*out = (voe_physics_shape){ .kind = collider->kind,
				    .centre = transform->position,
				    .rotation = transform->rotation,
				    .half = half };
	return true;
}
