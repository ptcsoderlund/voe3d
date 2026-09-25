// The drawables a pass holds back and draws later, in order. Internal to 3d:
// draw_system.c walks the mesh and panel tables, draws the world's solid
// meshes as it finds them and sets every other drawable aside here.
//
//     struct voe_3d_draw_group blended =
//             voe_3d_draw_group_new(arena, count, true);
//
//     voe_3d_draw_group_hold(&blended, entry, world_matrix, view.view);
//     ...
//     (void)voe_3d_draw_group_draw(device, &blended);
//
// FOUR GROUPS AND THREE OF THEM ARE HELD BACK. A drawable's layer says which
// side of the overlay's depth clear it is on and its alpha mode which pass on
// that side; only the world's solids can be drawn as they are found. Why, and
// why the blended ones are sorted furthest first, is draw_system.h's — its
// TWO PASSES and TWO LAYERS paragraphs — and is not repeated here.
//
// A group's arrays are the caller's arena's, pushed once by _new and given back
// by the caller's rewind; nothing here frees.
#pragma once

#include <3d/material_component.h>
#include <3d/shape_component.h>
#include <base/arena.h>
#include <ecs/component.h>
#include <render/device.h>
#include <scene/transform_component.h>

#include <stdbool.h>
#include <stdint.h>

// One entity, held back until its group's turn: everything that group needs in
// order to issue the draw without looking anything up again.
//
// IT IS ONE OF TWO THINGS AND `panel` SAYS WHICH. A mesh draw is a range in
// render's geometry pools plus the record it is shaded with; a panel draw is a
// range of this frame's element buffer plus the one matrix that puts those
// elements where the panel is. They go into the same groups, through the same
// sort, in one order — see voe_3d_draw_system_run in 3d/draw_system.h for why a
// separate pass for panels would be a bug rather than a simplification.
//
// A UNION AND NOT BOTH SETS OF FIELDS, because an entry is a hundred and forty
// bytes of matrices either way and every group is sized for every drawable in
// the world. What the two arms have in common is nothing: no field means the
// same thing in both, so there is nothing to hoist out of them.
struct voe_3d_deferred {
	bool panel;
	union {
		struct {
			voe_render_geometry geometry;
			voe_render_object object;
		} mesh;
		struct {
			// Element millimetres all the way to clip space:
			// projection × view × the transform's matrix × the
			// surface's own plane. Composed once, by the caller,
			// because render takes the finished product and cannot
			// compose it — it has never heard of a camera.
			voe_math_float4x4 transform;
			uint32_t first;
			uint32_t count;
		} elements;
	};
};

// One group of draws that could not be issued as the mesh table was walked,
// because it has to wait for a sort, for the depth clear, or for both.
//
// `depths` AND `order` ARE BOTH THERE OR BOTH ABSENT, AND THAT IS WHAT SAYS
// WHICH KIND OF GROUP THIS IS. A group with them sorts and draws blended; a
// group without them draws solid, in the order it was filled, which is table
// order. One field would do and two is what the sort already takes.
struct voe_3d_draw_group {
	struct voe_3d_deferred *deferred;
	float *depths;
	uint32_t *order;
	uint32_t count;
	// What it was sized for, kept so that _hold can say so. It is not read
	// anywhere else: the arrays are filled once and walked once, and `count`
	// is what says how far.
	uint32_t capacity;
};

struct voe_3d_draw_group voe_3d_draw_group_new(voe_base_arena *arena,
					       uint32_t capacity, bool sorted);
void voe_3d_draw_group_hold(struct voe_3d_draw_group *group,
			    struct voe_3d_deferred entry,
			    voe_math_float4x4 world, voe_math_float4x4 view);
bool voe_3d_draw_group_draw(voe_render_device *device,
			    const struct voe_3d_draw_group *group);

voe_render_object voe_3d_draw_group_object_of(const voe_scene_transform *transform,
					      const voe_3d_material *material,
					      const voe_3d_shape *shape,
					      voe_math_double3 eye);
bool voe_3d_draw_group_shape_type(const voe_ecs_world *world,
				  voe_ecs_type *out);
bool voe_3d_draw_group_is_the_same_entity(voe_ecs_entity a, voe_ecs_entity b);
