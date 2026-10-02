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
// Constraints: the table is looked for by a walk of the world's types, as
// pick.c's has_store does, because voe_ecs_component_type asserts on a key
// nothing registered; once per call. Its arena push never fails (base/arena.h),
// so the call has no false path today.
#include <3d/draw_system.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/double3.h>
#include <math/float3.h>
#include <scene/point_light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <stddef.h>

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
		};
	}

	frame->points = (voe_render_point_lights){ lights, filled };
	VOE_BASE_ASSERT(frame->points.count <= VOE_RENDER_POINT_LIGHTS,
			"more point lights than a pass carries");
	return true;
}
