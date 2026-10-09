// The held-back groups: room for one in the arena, an entity set aside in it
// with its sort key, and its draws issued sorted or in the order it was filled;
// with the three lookups the walk that fills them needs. See draw_group.h.
//
// Nothing here walks a table or clears depth: which group a drawable lands in,
// and when each group is drawn, is voe_3d_draw_system_run's.
#include "draw_group.h"
#include "draw_material.h"

#include <3d/depth_sort.h>
#include <3d/normal_matrix.h>
#include <base/assert.h>

// Whether two entity ids name the same entity. A zeroed id — which is what a
// frame that hides nothing carries — never matches a live one, because
// generation 0 is never handed out (ecs/world.h), so the hidden test needs no
// "is anything hidden at all" branch in front of it.
bool voe_3d_draw_group_is_the_same_entity(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// The view-space depth of an object's origin, which is the key the blended pass
// sorts on.
//
// THE ORIGIN IS THE WORLD MATRIX'S LAST COLUMN AND THAT IS NOT A SHORTCUT. A
// matrix applied to (0, 0, 0, 1) is its translation, and matrices here are
// row-major — m[row][column] — so the translation is m[0..2][3]. Reading it out
// costs three loads where the multiply would cost sixteen, and it is the same
// number.
//
// MORE NEGATIVE IS FURTHER AWAY, because a camera looks along its own −Z. The
// sign is not corrected here: voe_3d_depth_sort takes the view-space z as it is
// and there is exactly one place the convention is spelled out.
static float view_depth(voe_math_float4x4 view, voe_math_float4x4 world)
{
	voe_math_float4 origin = { world.m[0][3], world.m[1][3], world.m[2][3],
				   1.0f };

	return voe_math_float4x4_mul_float4(view, origin).z;
}

// The world's shape table, or false when it has none — dev registers none. A
// walk of the types rather than voe_ecs_component_type, which asserts on a key
// nothing registered; once per run, not per object.
bool voe_3d_draw_group_shape_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_3d_shape_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

// The record an entity is drawn with, which is the same two matrices and the
// same shading id whichever pass it ends up in; the world matrix is about `eye`.
// `shape` is the entity's shape or NULL; its colour, opaque, is the object's,
// and anything without one is drawn white — its material's colour as it is. A
// shape whose `material` names a loaded material in `models` (NULL for none)
// wears that record instead, white (0399 point 6).
voe_render_object voe_3d_draw_group_object_of(const voe_scene_transform *transform,
					      const voe_3d_material *material,
					      const voe_3d_shape *shape,
					      const voe_3d_models *models,
					      voe_math_double3 eye)
{
	voe_render_object object = { 0 };
	const voe_3d_material *worn =
		shape != NULL ? voe_3d_draw_material_named(models, shape->material) :
				NULL;

	if (worn != NULL) {
		material = worn;
		shape = NULL;
	}

	object.world = voe_scene_transform_matrix(*transform, eye);
	// One inverse per drawn object per frame, which is the cost of getting a
	// non-uniformly scaled thing lit correctly. It is computed rather than
	// stored for the same reason the world matrix is
	// (scene/transform_component.h): a second copy of the truth is a thing
	// to invalidate. If it ever measures slow it becomes a cached column in
	// the transform table and nothing here changes.
	object.normal = voe_3d_normal_matrix(object.world);
	object.shading = material->shading.index;
	object.colour = shape != NULL ?
				(voe_math_float4){ shape->colour.x,
						   shape->colour.y,
						   shape->colour.z, 1.0f } :
				(voe_math_float4){ 1.0f, 1.0f, 1.0f, 1.0f };

	return object;
}

// Room in the arena for one group, sized for the whole mesh table — see below
// for why that bound and not a measured one. A sorted group gets the three
// arrays the sort works in — the keys, the order and the merge sort's scratch;
// an unsorted one has nothing to sort and gets none of them.
//
// A TABLE WITH NOTHING IN IT PUSHES NOTHING. voe_base_arena_push asserts on a
// size of nought (base/arena.h), so an empty group is the zeroed struct and the
// fill and the draw below both do nothing with it.
//
// EACH GROUP'S SCRATCH IS SIZED BY THE WHOLE OF BOTH TABLES AND NOT BY WHAT
// LANDS IN IT. Which group a drawable is in is not known until both walks have
// finished, so the bound for each of them is every mesh and every panel there
// is; it is an arena, it is rewound at the end of the frame, and counting first
// would be a second walk to save memory that is given back a millisecond later.
// The overlay's solid group is the one exception and is sized by the meshes
// alone: a panel is blended and cannot land in it.
struct voe_3d_draw_group voe_3d_draw_group_new(voe_base_arena *arena,
					       uint32_t capacity, bool sorted)
{
	struct voe_3d_draw_group group = { 0 };

	if (capacity == 0)
		return group;

	group.capacity = capacity;
	group.deferred = voe_base_arena_push(
		arena, (size_t)capacity * sizeof(*group.deferred));
	if (sorted) {
		group.depths = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.depths));
		group.order = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.order));
		group.scratch = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.scratch));
	}
	return group;
}

// Sets one entity aside in its group. The view matrix rather than a depth,
// because only a sorted group has anywhere to put a key — so the work of
// computing one is not done at all for a group drawn in the order it was filled.
//
// `world` IS PASSED RATHER THAN READ OFF THE ENTRY, because the two kinds of
// entry keep their matrices in different places and neither of them keeps a
// world matrix as such — a panel's is already composed into a chain by the time
// it gets here. The key is the same key either way: the view-space depth of the
// object's origin.
void voe_3d_draw_group_hold(struct voe_3d_draw_group *group,
			    struct voe_3d_deferred entry,
			    voe_math_float4x4 world, voe_math_float4x4 view)
{
	// A group is sized for every drawable in the world, so a drawable that
	// exists always has room. It is asserted rather than assumed because the
	// size is now arithmetic over two tables: a group sized for nothing has
	// no arrays at all, and one sized for too few would write past an arena
	// push and corrupt whatever came after it. Either is a bug in the three
	// lines below the walk and not something a caller can cause.
	VOE_BASE_ASSERT(group->deferred != NULL &&
				group->count < group->capacity,
			"holding a drawable in a group that was not sized for it — see voe_3d_draw_group_new");

	group->deferred[group->count] = entry;
	if (group->depths != NULL)
		group->depths[group->count] = view_depth(view, world);
	group->count++;
}

// One group's draws: sorted furthest away first through the blended pipeline, or
// in the order it was filled through the solid one. Returns false only when a
// draw was refused, which stops this group and not the frame — the same rule the
// draws issued during the walk follow.
//
// A PANEL IS ISSUED FROM THE SAME LOOP AND IN THE SAME ORDER. Two loops, one
// over the meshes and one over the panels, would be two sorted lists laid end
// to end — which is not a sort, and which comes out right from most angles and
// wrong from the rest. The element draw's pipeline state is the blended
// pipeline's, so a panel and a see-through quad are the same kind of thing to
// sort and there is no reason to tell them apart here.
bool voe_3d_draw_group_draw(voe_render_device *device,
			    const struct voe_3d_draw_group *group)
{
	bool sorted = group->order != NULL;

	if (sorted)
		voe_3d_depth_sort(group->depths, group->count, group->order,
				  group->scratch);

	for (uint32_t i = 0; i < group->count; i++) {
		const struct voe_3d_deferred *drawn =
			&group->deferred[sorted ? group->order[i] : i];
		bool drawn_ok;

		// A panel ignores `sorted`: there is one element draw and it is
		// blended whichever group it landed in. A panel never reaches an
		// unsorted group anyway — see the walk — and the day one does,
		// drawing it correctly is better than drawing it as a mesh.
		if (drawn->panel)
			drawn_ok = voe_render_frame_draw_elements(
				device, drawn->elements.transform,
				drawn->elements.first, drawn->elements.count);
		else if (sorted)
			drawn_ok = voe_render_frame_draw_blended(
				device, drawn->mesh.geometry,
				drawn->mesh.object);
		else
			drawn_ok = voe_render_frame_draw(device,
							 drawn->mesh.geometry,
							 drawn->mesh.object);

		if (!drawn_ok)
			return false;
	}
	return true;
}
