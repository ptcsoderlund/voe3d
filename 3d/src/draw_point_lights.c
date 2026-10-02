// A pass's point lights out of the point light table (0320 point 6) — see
// voe_3d_draw_system_point_lights in 3d/draw_system.h for what is filled and
// what is left out.
//
// One walk of the table in its order into one array pushed at the table's count,
// capped at VOE_RENDER_POINT_LIGHTS, so a dark or unplaced light costs room it
// does not use. Each kept light's place is voe_scene_transform_between at the
// frame's lag, taken about the eye in double before it is narrowed (ADR-0250);
// its colour is the row's times its intensity and its falloff the row's, as
// authored (0321, 0322). A light of intensity 0 is left out.
//
// Then the shadow slots (0325 point 5): among the kept lights whose row casts,
// ranked by the eye's distance to the light's sphere, max(0, |position| −
// range), the nearest 16 take slots 1..16 nearest first, at strength
// 1 − smoothstep(FADE·D, D, distance), D the 17th's distance (none: strength 1);
// one at strength 0 takes no slot. The shadows call reads the slots from
// `frame->points`.
//
// Constraints: the table is looked for by a walk of the world's types, as
// pick.c's has_store does, because voe_ecs_component_type asserts on a key
// nothing registered; once per call. Its arena push never fails (base/arena.h),
// so the call has no false path today. The walk marks a casting light with
// `shadow` 1 until the choice replaces it; the choice keeps the 17 nearest in a
// sorted list on the stack, an insertion per light, 17 × 256 compares at most.
#include <3d/draw_system.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/double3.h>
#include <math/float3.h>
#include <scene/point_light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stddef.h>

// The 16 slots and the 17th whose distance D fades them.
#define RANKED (VOE_RENDER_POINT_SHADOWS + 1)

// Whether the world registered the point light table.
static bool has_point_lights(const voe_ecs_world *world)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++)
		if (voe_ecs_component_key(world,
					  voe_ecs_component_type_at(world, i)) ==
		    &voe_scene_point_light_key)
			return true;
	return false;
}

// 1 − smoothstep(FADE·D, D, distance). At or inside FADE·D it is 1 without the
// division, so a D of nought (the eye inside 17 spheres) divides by nothing.
static float shadow_strength(float distance, float far)
{
	float near = VOE_3D_POINT_SHADOW_FADE * far;

	if (distance <= near)
		return 1.0f;
	float t = fminf((distance - near) / (far - near), 1.0f);

	return 1.0f - t * t * (3.0f - 2.0f * t);
}

// The 17 nearest marked lights by distance to their sphere, then slots and
// strengths for the first 16 in that order; every other light's `shadow` is 0.
static void choose_shadows(voe_render_point_light *lights, uint32_t count)
{
	uint32_t ranked[RANKED];
	float distance[RANKED];
	uint32_t held = 0;

	for (uint32_t i = 0; i < count; i++) {
		if (lights[i].shadow == 0)
			continue;
		lights[i].shadow = 0;
		float d = fmaxf(0.0f, voe_math_float3_length(lights[i].position) -
					      lights[i].range);
		uint32_t at = held < RANKED ? held++ : RANKED;

		while (at > 0 && distance[at - 1] > d) {
			if (at < RANKED) {
				ranked[at] = ranked[at - 1];
				distance[at] = distance[at - 1];
			}
			at--;
		}
		if (at < RANKED) {
			ranked[at] = i;
			distance[at] = d;
		}
	}

	float far = held == RANKED ? distance[RANKED - 1] : INFINITY;

	for (uint32_t k = 0; k < held && k < VOE_RENDER_POINT_SHADOWS; k++) {
		float strength = shadow_strength(distance[k], far);

		if (!(strength > 0.0f))
			break;
		lights[ranked[k]].shadow = k + 1;
		lights[ranked[k]].shadow_strength = strength;
	}
}

bool voe_3d_draw_system_point_lights(const voe_ecs_world *world,
				     voe_3d_frame *frame, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(world != NULL, "point lights of no world");
	VOE_BASE_ASSERT(frame != NULL, "point lights into no frame");
	VOE_BASE_ASSERT(arena != NULL, "point lights into no arena");

	frame->points = (voe_render_point_lights){ 0 };
	if (!has_point_lights(world))
		return true;

	uint32_t count = voe_scene_point_light_count(world);
	uint32_t room = count < VOE_RENDER_POINT_LIGHTS ? count :
							  VOE_RENDER_POINT_LIGHTS;
	if (room == 0)
		return true;

	const voe_scene_point_light *rows = voe_scene_point_light_rows(world);
	const voe_ecs_entity *owners = voe_scene_point_light_entities(world);
	voe_render_point_light *lights =
		voe_base_arena_push(arena, sizeof(*lights) * room);
	uint32_t filled = 0;

	for (uint32_t i = 0; i < count && filled < room; i++) {
		float intensity = rows[i].intensity;

		if (!(intensity > 0.0f) ||
		    voe_scene_transform_get(world, owners[i]) == NULL)
			continue;
		voe_scene_transform place = voe_scene_transform_between(
			world, owners[i], frame->lag);
		lights[filled++] = (voe_render_point_light){
			.position = voe_math_double3_to_float3(
				voe_math_double3_sub(place.position, frame->eye)),
			.range = rows[i].range,
			.colour = voe_math_float3_scale(rows[i].colour, intensity),
			.falloff = rows[i].falloff,
			.shadow = rows[i].cast_shadows ? 1u : 0u,
		};
	}
	choose_shadows(lights, filled);

	frame->points = (voe_render_point_lights){ lights, filled };
	VOE_BASE_ASSERT(frame->points.count <= VOE_RENDER_POINT_LIGHTS,
			"more point lights than a pass carries");
	return true;
}
