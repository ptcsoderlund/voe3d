// A pass's light blockers out of the light blocker table (0347 point 2) — see
// voe_3d_draw_system_light_blockers in 3d/draw_system.h for what is filled and
// what is left out.
//
// One walk of the table in its order into one array pushed at the table's
// count, capped at VOE_RENDER_LIGHT_BLOCKERS, so an unplaced or flat blocker
// costs room it does not use. Each kept box is voe_3d_light_blocker_shape at
// the frame's lag, its centre taken about the eye in double before it is
// narrowed (ADR-0250); its rows are render/device.h's (a_i / h_i,
// −a_i·c / h_i), a_i the rotated local axes, and its sphere is the centre and
// the half's length, the box's corner. A half of nought on any axis would
// divide by nothing and holds no point, so that box is left out.
//
// Constraints: the table is looked for by a walk of the world's types, as
// draw_point_lights.c's has_point_lights does, because
// voe_ecs_component_type asserts on a key nothing registered; once per call.
// Its arena push never fails (base/arena.h), so the call has no false path
// today.
#include <3d/draw_system.h>
#include <3d/light_blocker.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/double3.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <scene/light_blocker_component.h>

#include <stddef.h>

// Whether the world registered the light blocker table.
static bool has_light_blockers(const voe_ecs_world *world)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++)
		if (voe_ecs_component_key(world,
					  voe_ecs_component_type_at(world, i)) ==
		    &voe_scene_light_blocker_key)
			return true;
	return false;
}

// One row of the box's unit space: `axis` over `half`, its w −axis·centre
// over `half`.
static voe_math_float4 row_of(voe_math_float3 axis, float half,
			      voe_math_float3 centre)
{
	return (voe_math_float4){ axis.x / half, axis.y / half, axis.z / half,
				  -voe_math_float3_dot(axis, centre) / half };
}

// The pass's record of `box`, about `eye`.
static voe_render_light_blocker record_of(voe_physics_shape box,
					  voe_math_double3 eye)
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

	return (voe_render_light_blocker){
		.rows = { row_of(x, box.half.x, centre),
			  row_of(y, box.half.y, centre),
			  row_of(z, box.half.z, centre) },
		.sphere = { centre.x, centre.y, centre.z,
			    voe_math_float3_length(box.half) },
	};
}

bool voe_3d_draw_system_light_blockers(const voe_ecs_world *world,
				       voe_3d_frame *frame,
				       voe_base_arena *arena)
{
	VOE_BASE_ASSERT(world != NULL, "light blockers of no world");
	VOE_BASE_ASSERT(frame != NULL, "light blockers into no frame");
	VOE_BASE_ASSERT(arena != NULL, "light blockers into no arena");

	frame->blockers = (voe_render_light_blockers){ 0 };
	if (!has_light_blockers(world))
		return true;

	uint32_t count = voe_scene_light_blocker_count(world);
	uint32_t room = count < VOE_RENDER_LIGHT_BLOCKERS ?
				count :
				VOE_RENDER_LIGHT_BLOCKERS;
	if (room == 0)
		return true;

	const voe_ecs_entity *owners = voe_scene_light_blocker_entities(world);
	voe_render_light_blocker *records =
		voe_base_arena_push(arena, sizeof(*records) * room);
	uint32_t filled = 0;

	for (uint32_t i = 0; i < count && filled < room; i++) {
		voe_physics_shape box;

		if (!voe_3d_light_blocker_shape(world, owners[i], frame->lag,
						&box) ||
		    !(box.half.x > 0.0f && box.half.y > 0.0f &&
		      box.half.z > 0.0f))
			continue;
		records[filled++] = record_of(box, frame->eye);
	}

	frame->blockers = (voe_render_light_blockers){ records, filled };
	return true;
}
