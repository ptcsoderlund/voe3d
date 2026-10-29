// The world's clouds as a pass takes them (0492 point 3):
// voe_3d_draw_system_clouds, declared in 3d/draw_system.h, which _frame calls
// with the frame's eye and a view with its own eye calls as it calls _sky.
//
// THE FIRST CLOUDS ROW WHOSE ENTITY IS ON (0440), an off row, itself or through
// a parent, being as no row; no table, no row or none on is the zeroed record,
// which render draws as no clouds. The kind maps Flat and Volumetric onto
// render's own numbers, which differ from scene's by one: render's 0 is none.
//
// EVERY METRE ABOUT THE EYE IS TAKEN IN DOUBLE. `bottom` is the row's height −
// the eye's y; `origin` is the eye's x and z less the drift's, wrapped into 0 to
// VOE_RENDER_CLOUDS_PERIOD before it becomes a float, so a far eye or a long
// wind keeps its precision. A row with no drift yet, before the clouds system
// first runs, has moved nowhere.
#include <3d/draw_system.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <scene/clouds_component.h>
#include <scene/identity_component.h>

#include <math.h>

// Whether the world registered the table under `key`: walked for, because
// voe_ecs_component_type asserts on a key nothing registered.
static bool has_table(const voe_ecs_world *world, const struct voe_ecs_key *key)
{
	uint32_t types;

	VOE_BASE_ASSERT(world != NULL && key != NULL,
			"asking for a table of no world or no key");
	types = voe_ecs_component_type_count(world);
	for (uint32_t i = 0; i < types; i++)
		if (voe_ecs_component_key(world, voe_ecs_component_type_at(world, i)) ==
		    key)
			return true;
	return false;
}

// `metres` into 0 to VOE_RENDER_CLOUDS_PERIOD, in double, then as a float that
// stays below the period: a value a hair under it rounds up to it in float, and
// the field there is the field at 0.
static float wrapped(double metres)
{
	double period = (double)VOE_RENDER_CLOUDS_PERIOD;
	double inside = fmod(metres, period);
	float answer;

	VOE_BASE_ASSERT(isfinite(metres), "wrapping a place that is no number");
	if (inside < 0.0)
		inside += period;
	answer = (float)inside;
	if (answer >= VOE_RENDER_CLOUDS_PERIOD)
		answer = 0.0f;
	VOE_BASE_ASSERT(answer >= 0.0f && answer < VOE_RENDER_CLOUDS_PERIOD,
			"a wrapped place outside the period");
	return answer;
}

voe_render_clouds voe_3d_draw_system_clouds(const voe_ecs_world *world,
					    voe_math_double3 eye)
{
	const voe_ecs_entity *owners;
	const voe_scene_clouds_drift *drift;
	voe_scene_clouds row;
	uint32_t count;
	uint32_t at = 0;
	double x;
	double z;

	VOE_BASE_ASSERT(world != NULL, "the clouds of no world");
	VOE_BASE_ASSERT(isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z),
			"the clouds about an eye at no place");
	count = has_table(world, &voe_scene_clouds_key) ?
			voe_scene_clouds_count(world) :
			0;
	owners = count > 0 ? voe_scene_clouds_entities(world) : NULL;
	while (at < count && voe_scene_identity_off(world, owners[at]))
		at++;
	if (at >= count)
		return (voe_render_clouds){ 0 };
	row = voe_scene_clouds_rows(world)[at];
	drift = voe_scene_clouds_drift_get(world, owners[at]);
	x = eye.x - (drift != NULL ? drift->x : 0.0);
	z = eye.z - (drift != NULL ? drift->z : 0.0);
	return (voe_render_clouds){
		.kind = row.kind == VOE_SCENE_CLOUDS_VOLUMETRIC ?
				VOE_RENDER_CLOUDS_VOLUMETRIC :
				VOE_RENDER_CLOUDS_FLAT,
		.coverage = row.coverage,
		.bottom = (float)((double)row.height - eye.y),
		.thickness = row.thickness,
		.origin = { wrapped(x), wrapped(z) },
		.shadow_strength = row.shadow_strength,
	};
}
