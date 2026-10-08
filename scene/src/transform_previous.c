// The previous step's transforms: the table a stepping world registers, the
// copy taken at the start of each step, the blend the draw reads — each
// link of the parent chain blended by the lag, then composed into a world
// transform (0281 point 2) — and whether an entity was remembered at all.
//
// THE TABLE IS FOUND BY WALKING THE WORLD'S TYPES, because a world that does not
// step never registered it and voe_ecs_component_type asserts on a key nobody
// registered. between() treats "not registered" as "nothing to blend towards".
//
// A NAIVE WALK: remember() copies every transform, moved or not, and looks each
// previous row up by entity. That is one lookup per transform per step; a dirty
// flag on the transform table would lift it if a step ever shows it.
//
// A PREVIOUS ROW OUTLIVES ITS TRANSFORM. Nothing here removes one, and between()
// reads a previous row only for an entity whose transform exists, so a stale row
// is never drawn.
#include <base/assert.h>
#include <scene/parent_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stddef.h>

static const struct voe_ecs_key previous_key = {
	"voe_scene_transform_previous"
};

// The previous table, or false when this world never registered one.
static bool previous_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &previous_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

void voe_scene_transform_previous_register(voe_ecs_world *world,
					   uint32_t capacity)
{
	voe_ecs_type type;

	VOE_BASE_ASSERT(world != NULL, "registering previous transforms in no world");
	VOE_BASE_ASSERT(!previous_type(world, &type),
			"registering previous transforms twice");

	(void)voe_ecs_component_register(world, &previous_key,
					 sizeof(voe_scene_transform), capacity,
					 &voe_ecs_runtime_only);
}

void voe_scene_transform_remember(voe_ecs_world *world)
{
	voe_ecs_type previous;
	voe_ecs_type current;
	const voe_scene_transform *rows;
	const voe_ecs_entity *owners;
	uint32_t count;

	VOE_BASE_ASSERT(world != NULL, "remembering transforms of no world");
	VOE_BASE_ASSERT(previous_type(world, &previous),
			"remembering transforms in a world that registered no previous table");

	current = voe_ecs_component_type(world, &voe_scene_transform_key);
	rows = voe_ecs_component_rows(world, current);
	owners = voe_ecs_component_entities(world, current);
	count = voe_ecs_component_count(world, current);

	for (uint32_t i = 0; i < count; i++) {
		bool kept = voe_ecs_component_get(world, previous, owners[i]) != NULL
				    ? voe_ecs_component_set(world, previous,
							    owners[i], &rows[i])
				    : voe_ecs_component_add(world, previous,
							    owners[i], &rows[i]);

		VOE_BASE_ASSERT(kept,
				"the previous transform table is smaller than the transform table");
	}
}

static double blend_double(double now, double then, double lag)
{
	return now + (then - now) * lag;
}

static float blend_float(float now, float then, float lag)
{
	return now + (then - now) * lag;
}

// The shorter of the two ways round: q and -q are one rotation, so the
// remembered one is flipped when it points away from the current one.
static voe_math_quat blend_rotation(voe_math_quat now, voe_math_quat then,
				    float lag)
{
	float dot = now.x * then.x + now.y * then.y + now.z * then.z +
		    now.w * then.w;
	float sign = dot < 0.0f ? -1.0f : 1.0f;
	voe_math_quat q = {
		blend_float(now.x, sign * then.x, lag),
		blend_float(now.y, sign * then.y, lag),
		blend_float(now.z, sign * then.z, lag),
		blend_float(now.w, sign * then.w, lag),
	};

	return voe_math_quat_normalize(q);
}

// One row `lag` back: the current row when the entity was never remembered.
static voe_scene_transform blended_row(const voe_ecs_world *world,
				       voe_ecs_type previous,
				       voe_ecs_entity entity, float lag)
{
	const voe_scene_transform *now = voe_scene_transform_get(world, entity);
	const voe_scene_transform *then;
	voe_scene_transform out;

	VOE_BASE_ASSERT(now != NULL, "blending an entity with no transform");

	then = voe_ecs_component_get(world, previous, entity);
	if (then == NULL)
		return *now;

	out.position = (voe_math_double3){
		blend_double(now->position.x, then->position.x, lag),
		blend_double(now->position.y, then->position.y, lag),
		blend_double(now->position.z, then->position.z, lag),
	};
	out.rotation = blend_rotation(now->rotation, then->rotation, lag);
	out.scale = (voe_math_float3){
		blend_float(now->scale.x, then->scale.x, lag),
		blend_float(now->scale.y, then->scale.y, lag),
		blend_float(now->scale.z, then->scale.z, lag),
	};
	return out;
}

// The parent a row is relative to, false for a root: the same end of the chain
// as voe_scene_transform_world's.
static bool parent_of(const voe_ecs_world *world, voe_ecs_entity entity,
		      voe_ecs_entity *out)
{
	const voe_scene_parent *row = voe_scene_parent_get(world, entity);

	if (row == NULL || voe_scene_transform_get(world, row->parent) == NULL)
		return false;
	*out = row->parent;
	return true;
}

voe_scene_transform voe_scene_transform_between(const voe_ecs_world *world,
						voe_ecs_entity entity,
						float lag)
{
	voe_ecs_type previous;
	voe_scene_transform placed;
	voe_ecs_entity at = entity;

	VOE_BASE_ASSERT(world != NULL, "blending a transform in no world");
	VOE_BASE_ASSERT(lag >= 0.0f && lag <= 1.0f,
			"a lag is between nought and one step");

	if (lag == 0.0f || !previous_type(world, &previous))
		return voe_scene_transform_world(world, entity);

	placed = blended_row(world, previous, entity, lag);
	for (uint32_t link = 0; link < VOE_SCENE_PARENT_DEPTH_MAX; link++) {
		if (!parent_of(world, at, &at))
			break;
		placed = voe_scene_transform_compose(
			blended_row(world, previous, at, lag), placed);
	}

	VOE_BASE_DEBUG_ASSERT(isfinite(placed.position.x),
			      "a blended world place that is not finite");
	return placed;
}

// No assert by design: 3d asks this of every caster, in worlds with and
// without a previous table, and a missing world or table is simply "no".
bool voe_scene_transform_remembered(const voe_ecs_world *world,
				    voe_ecs_entity entity)
{
	voe_ecs_type previous;

	if (world == NULL || !previous_type(world, &previous))
		return false;
	return voe_ecs_component_get(world, previous, entity) != NULL;
}
