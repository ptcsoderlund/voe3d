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
// THE KINDS ARE MASKS BY KEPT INDEX (0350 point 2): a kept record's bit is its
// place among the kept, not the table's, so a box left out shifts the bits
// after it. A Wall's goes in `walls`, an Indoors' in `indoors`, a Room's in
// neither. `sun` is voe_render_light_blockers_mask over the kept records at
// the light row's entity's world place at the lag, about the eye in float as a
// box's centre is; no light table, no row or no transform is 0.
//
// Constraints: the tables are looked for by a walk of the world's types, as
// draw_point_lights.c's has_point_lights does, because
// voe_ecs_component_type asserts on a key nothing registered; twice per call.
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
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <stddef.h>

// Whether the world registered the table of `key`.
static bool has_table(const voe_ecs_world *world, const struct voe_ecs_key *key)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++)
		if (voe_ecs_component_key(world,
					  voe_ecs_component_type_at(world, i)) ==
		    key)
			return true;
	return false;
}

// The mask of `records` holding the light row's entity's place, 0 for none.
static uint32_t sun_mask(const voe_ecs_world *world, const voe_3d_frame *frame,
			 const voe_render_light_blocker *records,
			 uint32_t count)
{
	if (!has_table(world, &voe_scene_light_key) ||
	    voe_scene_light_count(world) == 0)
		return 0;
	voe_ecs_entity lit = voe_scene_light_entities(world)[0];

	if (voe_scene_transform_get(world, lit) == NULL)
		return 0;
	voe_scene_transform place =
		voe_scene_transform_between(world, lit, frame->lag);

	return voe_render_light_blockers_mask(
		records, count,
		voe_math_double3_to_float3(
			voe_math_double3_sub(place.position, frame->eye)));
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
	if (!has_table(world, &voe_scene_light_blocker_key))
		return true;

	uint32_t count = voe_scene_light_blocker_count(world);
	uint32_t room = count < VOE_RENDER_LIGHT_BLOCKERS ?
				count :
				VOE_RENDER_LIGHT_BLOCKERS;
	if (room == 0)
		return true;

	const voe_ecs_entity *owners = voe_scene_light_blocker_entities(world);
	const voe_scene_light_blocker *rows = voe_scene_light_blocker_rows(world);
	voe_render_light_blocker *records =
		voe_base_arena_push(arena, sizeof(*records) * room);
	uint32_t filled = 0;
	uint32_t walls = 0;
	uint32_t indoors = 0;

	for (uint32_t i = 0; i < count && filled < room; i++) {
		voe_physics_shape box;

		if (!voe_3d_light_blocker_shape(world, owners[i], frame->lag,
						&box) ||
		    !(box.half.x > 0.0f && box.half.y > 0.0f &&
		      box.half.z > 0.0f))
			continue;
		if (rows[i].kind == VOE_SCENE_LIGHT_BLOCKER_WALL)
			walls |= 1u << filled;
		else if (rows[i].kind == VOE_SCENE_LIGHT_BLOCKER_INDOORS)
			indoors |= 1u << filled;
		records[filled++] = record_of(box, frame->eye);
	}

	frame->blockers = (voe_render_light_blockers){
		.blockers = records,
		.count = filled,
		.walls = walls,
		.indoors = indoors,
		.sun = sun_mask(world, frame, records, filled),
	};
	return true;
}
