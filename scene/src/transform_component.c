// The transform component: its key, the matrix its three numbers become,
// composing one under a parent and back, the world place up the parent chain,
// and the reads anyone may do.
//
// NOTHING IN THIS FILE WRITES. Every write to this table is in
// transform_system.c, which is the whole of what "read by anyone, written by
// one" costs to arrange.
//
// THE QUATERNION HELPERS ARE STATIC HERE, NOT IN math. math/quat.h has no
// conjugate and no rotate-a-vector, and the rotate wanted here is in double for
// a position, which is this module's need rather than a general one.
#include <base/assert.h>
#include <scene/parent_component.h>
#include <scene/transform_component.h>

#include <math.h>
#include <stdbool.h>

const struct voe_ecs_key voe_scene_transform_key = { "voe_scene_transform" };

static voe_math_quat conjugate(voe_math_quat q)
{
	return (voe_math_quat){ -q.x, -q.y, -q.z, q.w };
}

// v + 2w (q × v) + 2 q × (q × v), for a unit q, every step in double so a
// position far from the origin comes out as exact as it went in.
static voe_math_double3 rotated(voe_math_quat q, voe_math_double3 v)
{
	double x = q.x, y = q.y, z = q.z, w = q.w;
	double tx = 2.0 * (y * v.z - z * v.y);
	double ty = 2.0 * (z * v.x - x * v.z);
	double tz = 2.0 * (x * v.y - y * v.x);

	VOE_BASE_DEBUG_ASSERT(fabs(voe_math_quat_length(q) - 1.0f) <=
				      VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE,
			      "rotating by a quaternion that is not unit");

	return (voe_math_double3){
		v.x + w * tx + (y * tz - z * ty),
		v.y + w * ty + (z * tx - x * tz),
		v.z + w * tz + (x * ty - y * tx),
	};
}

// a / b, or zero where b is zero: nothing composed under a flattened axis can
// be told apart along it, so zero is as good an answer as any and is finite.
static double divided(double a, float b)
{
	return b == 0.0f ? 0.0 : a / b;
}

static bool is_finite(voe_scene_transform t)
{
	return isfinite(t.position.x) && isfinite(t.position.y) &&
	       isfinite(t.position.z) && isfinite(t.rotation.x) &&
	       isfinite(t.rotation.y) && isfinite(t.rotation.z) &&
	       isfinite(t.rotation.w) && isfinite(t.scale.x) &&
	       isfinite(t.scale.y) && isfinite(t.scale.z);
}

voe_scene_transform voe_scene_transform_compose(voe_scene_transform parent,
						voe_scene_transform child)
{
	VOE_BASE_DEBUG_ASSERT(is_finite(parent) && is_finite(child),
			      "composing a transform that is not finite");

	voe_math_double3 scaled = {
		child.position.x * parent.scale.x,
		child.position.y * parent.scale.y,
		child.position.z * parent.scale.z,
	};
	voe_scene_transform world = {
		.position = voe_math_double3_add(
			parent.position, rotated(parent.rotation, scaled)),
		.rotation = voe_math_quat_normalize(
			voe_math_quat_mul(parent.rotation, child.rotation)),
		.scale = voe_math_float3_mul(parent.scale, child.scale),
	};

	VOE_BASE_DEBUG_ASSERT(is_finite(world), "a composed transform overflowed");
	return world;
}

voe_scene_transform voe_scene_transform_relative(voe_scene_transform parent,
						 voe_scene_transform placed)
{
	VOE_BASE_DEBUG_ASSERT(is_finite(parent) && is_finite(placed),
			      "relating a transform that is not finite");

	voe_math_quat undo = conjugate(parent.rotation);
	voe_math_double3 local = rotated(
		undo, voe_math_double3_sub(placed.position, parent.position));
	voe_scene_transform relative = {
		.position = { divided(local.x, parent.scale.x),
			      divided(local.y, parent.scale.y),
			      divided(local.z, parent.scale.z) },
		.rotation = voe_math_quat_normalize(
			voe_math_quat_mul(undo, placed.rotation)),
		.scale = { (float)divided(placed.scale.x, parent.scale.x),
			   (float)divided(placed.scale.y, parent.scale.y),
			   (float)divided(placed.scale.z, parent.scale.z) },
	};

	VOE_BASE_DEBUG_ASSERT(is_finite(relative),
			      "a relative transform overflowed");
	return relative;
}

voe_math_float4x4 voe_scene_transform_matrix(voe_scene_transform transform,
					      voe_math_double3 origin)
{
	// Subtracted in double, narrowed after: the difference is small, the two
	// positions are not.
	voe_math_float3 offset = voe_math_double3_to_float3(
		voe_math_double3_sub(transform.position, origin));

	// T · R · S. Right to left: scaled, then turned, then moved.
	return voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(offset),
		voe_math_float4x4_mul(
			voe_math_float4x4_from_quat(transform.rotation),
			voe_math_float4x4_from_scale(transform.scale)));
}

const voe_scene_transform *voe_scene_transform_get(const voe_ecs_world *world,
						   voe_ecs_entity entity)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "reading a transform out of no world");

	return voe_ecs_component_get(
		world, voe_ecs_component_type(world, &voe_scene_transform_key),
		entity);
}

// The parent whose transform the row is relative to, false for a root: no row,
// or a parent dead or without a transform.
static bool placed_under(const voe_ecs_world *world, voe_ecs_entity entity,
			 voe_ecs_entity *out)
{
	const voe_scene_parent *row = voe_scene_parent_get(world, entity);

	if (row == NULL || voe_scene_transform_get(world, row->parent) == NULL)
		return false;
	*out = row->parent;
	return true;
}

voe_scene_transform voe_scene_transform_world(const voe_ecs_world *world,
					      voe_ecs_entity entity)
{
	const voe_scene_transform *row = voe_scene_transform_get(world, entity);
	voe_scene_transform placed;
	voe_ecs_entity at = entity;

	VOE_BASE_ASSERT(row != NULL, "the world place of no transform");

	placed = *row;
	for (uint32_t link = 0; link < VOE_SCENE_PARENT_DEPTH_MAX; link++) {
		if (!placed_under(world, at, &at))
			break;
		placed = voe_scene_transform_compose(
			*voe_scene_transform_get(world, at), placed);
	}

	VOE_BASE_DEBUG_ASSERT(isfinite(placed.position.x),
			      "a world place that is not finite");
	return placed;
}

voe_scene_transform voe_scene_transform_local(const voe_ecs_world *world,
					      voe_ecs_entity entity,
					      voe_scene_transform placed)
{
	voe_ecs_entity parent;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "placing in no world");

	if (!placed_under(world, entity, &parent))
		return placed;
	return voe_scene_transform_relative(
		voe_scene_transform_world(world, parent), placed);
}

uint32_t voe_scene_transform_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}

const voe_scene_transform *voe_scene_transform_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}

const voe_ecs_entity *voe_scene_transform_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(
		world, voe_ecs_component_type(world, &voe_scene_transform_key));
}
