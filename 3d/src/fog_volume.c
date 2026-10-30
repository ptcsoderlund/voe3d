// A world's fog volumes as a pass's records — see 3d/fog_volume.h.
//
// The shape looks the row and the transform up before anything is computed,
// so a missing one leaves *box as it was; the place is
// voe_scene_transform_between at the lag, so a parent carries the box.
//
// The records are one walk of the table in its order into render's fixed
// array, skipping an off owner (0428 point 4), a missing transform, a flat box
// and a thickness of 0. A record's rows are render/device.h's (a_i / h_i,
// −a_i·c / h_i), the centre taken about the eye in double before it is
// narrowed, written as draw_light_blockers.c writes a blocker's.
#include <3d/fog_volume.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <physics/collider_component.h>
#include <scene/fog_volume_component.h>
#include <scene/identity_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stddef.h>

bool voe_3d_fog_volume_shape(const voe_ecs_world *world, voe_ecs_entity entity,
			     float lag, voe_physics_shape *box)
{
	VOE_BASE_ASSERT(world != NULL, "a fog volume of no world");
	VOE_BASE_ASSERT(box != NULL, "a fog volume into nothing");

	const voe_scene_fog_volume *row = voe_scene_fog_volume_get(world, entity);

	if (row == NULL || voe_scene_transform_get(world, entity) == NULL)
		return false;
	voe_scene_transform place =
		voe_scene_transform_between(world, entity, lag);

	*box = (voe_physics_shape){
		.kind = VOE_PHYSICS_COLLIDER_BOX,
		.centre = place.position,
		.rotation = place.rotation,
		.half = { fabsf(place.scale.x) * row->size.x * 0.5f,
			  fabsf(place.scale.y) * row->size.y * 0.5f,
			  fabsf(place.scale.z) * row->size.z * 0.5f },
	};
	return true;
}

// Whether the world registered the fog volume table: walked for, because
// voe_ecs_component_type asserts on a key nothing registered.
static bool has_fog_volumes(const voe_ecs_world *world)
{
	uint32_t types = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < types; i++)
		if (voe_ecs_component_key(world, voe_ecs_component_type_at(world, i)) ==
		    &voe_scene_fog_volume_key)
			return true;
	return false;
}

// `metres` into 0 to `period`, in double, then as a float that stays below
// the period: a value a hair under it rounds up to it in float, and the field
// there is the field at 0.
static float wrapped(double metres, float period)
{
	double inside = fmod(metres, (double)period);
	float answer;

	VOE_BASE_ASSERT(isfinite(metres), "wrapping a place that is no number");
	if (inside < 0.0)
		inside += (double)period;
	answer = (float)inside;
	if (answer >= period)
		answer = 0.0f;
	VOE_BASE_ASSERT(answer >= 0.0f && answer < period,
			"a wrapped place outside the period");
	return answer;
}

// One row of the box's unit space: `axis` over `half`, its w −axis·centre
// over `half`.
static voe_math_float4 row_of(voe_math_float3 axis, float half,
			      voe_math_float3 centre)
{
	return (voe_math_float4){ axis.x / half, axis.y / half, axis.z / half,
				  -voe_math_float3_dot(axis, centre) / half };
}

// The record of `box` about `eye`, its fields from `row` and `drift`.
static voe_render_fog_volume record_of(voe_physics_shape box,
				       voe_math_double3 eye,
				       const voe_scene_fog_volume *row,
				       voe_scene_fog_volume_drift drift)
{
	voe_math_float4x4 turn = voe_math_float4x4_from_quat(box.rotation);
	voe_math_float3 centre = voe_math_double3_to_float3(
		voe_math_double3_sub(box.centre, eye));
	voe_math_float3 x = voe_math_float4x4_transform_dir(
		turn, (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	voe_math_float3 y = voe_math_float4x4_transform_dir(
		turn, (voe_math_float3){ 0.0f, 1.0f, 0.0f });
	voe_math_float3 z = voe_math_float4x4_transform_dir(
		turn, (voe_math_float3){ 0.0f, 0.0f, 1.0f });
	double moved = sqrt(drift.x * drift.x + drift.z * drift.z);

	return (voe_render_fog_volume){
		.rows = { row_of(x, box.half.x, centre),
			  row_of(y, box.half.y, centre),
			  row_of(z, box.half.z, centre) },
		.patches = { wrapped(eye.x - drift.x, VOE_RENDER_FOG_PATCHES),
			     wrapped(eye.y, VOE_RENDER_FOG_PATCHES),
			     wrapped(eye.z - drift.z, VOE_RENDER_FOG_PATCHES) },
		.density = 0.1f * row->thickness * row->thickness,
		.change = { wrapped(eye.x - 1.5 * drift.x, VOE_RENDER_FOG_CHANGE),
			    wrapped(eye.y - 0.25 * moved, VOE_RENDER_FOG_CHANGE),
			    wrapped(eye.z - 1.5 * drift.z, VOE_RENDER_FOG_CHANGE) },
		.patchiness = row->patchiness,
		.godrays = row->godrays_strength,
	};
}

voe_render_fog voe_3d_fog_volumes(const voe_ecs_world *world,
				  voe_math_double3 eye, float lag)
{
	voe_render_fog fog = { 0 };

	VOE_BASE_ASSERT(world != NULL, "the fog of no world");
	VOE_BASE_ASSERT(isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z),
			"the fog about an eye at no place");
	if (!has_fog_volumes(world))
		return fog;

	uint32_t count = voe_scene_fog_volume_count(world);
	const voe_ecs_entity *owners =
		count > 0 ? voe_scene_fog_volume_entities(world) : NULL;
	const voe_scene_fog_volume *rows =
		count > 0 ? voe_scene_fog_volume_rows(world) : NULL;

	for (uint32_t i = 0; i < count && fog.count < VOE_RENDER_FOG_VOLUMES;
	     i++) {
		voe_physics_shape box;
		const voe_scene_fog_volume_drift *drift;

		if (rows[i].thickness <= 0.0f ||
		    voe_scene_identity_off(world, owners[i]) ||
		    !voe_3d_fog_volume_shape(world, owners[i], lag, &box) ||
		    !(box.half.x > 0.0f && box.half.y > 0.0f && box.half.z > 0.0f))
			continue;
		drift = voe_scene_fog_volume_drift_get(world, owners[i]);
		fog.volumes[fog.count++] = record_of(
			box, eye, &rows[i],
			drift != NULL ? *drift : (voe_scene_fog_volume_drift){ 0 });
	}
	VOE_BASE_ASSERT(fog.count <= VOE_RENDER_FOG_VOLUMES,
			"more fog volumes than a pass holds");
	return fog;
}
