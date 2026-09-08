// The draw system: the camera, the sun, the matrices each object is drawn with,
// and one draw per mesh into the frame the loop has opened.
//
// IT WALKS THE MESH TABLE AND LOOKS THE OTHER TWO COMPONENTS UP BY ENTITY. That
// is what a flat-table ECS with no archetypes costs and it is the trade this
// engine took: the walk over the meshes is linear and the two lookups are one
// load each (ecs/component.h). If that ever measures slow it is a later card
// with a number attached, and nothing above this line changes.
//
// THE CAMERA AND THE SUN ARE READ THE SAME WAY AND BOTH ARE REQUIRED. Row zero
// of each table, because there is exactly one of each — see the header for why
// more than one is a mistake rather than a choice, and why a world with no sun
// asserts here rather than drawing something black.
//
// A DRAW THAT IS REFUSED STOPS ITS GROUP BUT NOT THE FRAME. Running out of room
// for objects means the device was made for fewer than this scene has; the loop
// still ends and presents the frame, so a person sees most of the scene and a
// line on stderr rather than a black window — which is also why this returns
// nothing: there is no answer here the loop should act on. The alternative —
// abandoning the recording — would leave the slot's fence unsignalled.
//
// THE CAMERA IS WORKED OUT IN voe_3d_draw_system_frame AND NOWHERE ELSE. The
// loop needs it before _begin and this system needs its view for the sort, so
// the one function computes it and the loop carries the answer to both. Nothing
// in _run reads the camera table.
//
// THE LATER GROUPS ARE BUILT ON THE WAY THROUGH THE FIRST ONE AND NOT BY MORE
// WALKS. An entity that cannot be drawn where it is found has its record and its
// geometry set aside as the mesh table is walked, together with the depth to sort
// on where its group sorts, so the two lookups it costs happen once. What is set
// aside is the record itself rather than the row, because a later pass would
// otherwise look the same two components up again.
//
// THERE ARE FOUR GROUPS AND ONLY ONE OF THEM CAN BE DRAWN AS IT IS FOUND. The
// layer says which side of the depth clear an object is on and the alpha mode
// says which pass it is in on that side, and the two are independent — so the
// walk classifies into world-solid, world-blended, overlay-solid and
// overlay-blended, and issues the first of those immediately. Nothing that comes
// later can get in front of a world-solid object: the depth buffer resolves it
// per pixel, and the clear has not happened yet.
//
// THE OVERLAY HAS A SOLID GROUP FROM THE START AND THAT WAS A CHOICE. Text is
// blended, so the overlay's blended group is what the first user of this needs
// and the solid one could have waited. It is here because the layer and the alpha
// mode are separate axes: leaving the solid group out would mean an opaque
// drawable marked overlay drew in the world instead, silently and correctly
// enough to look like nothing was wrong. It costs one branch in a walk that is
// already happening.
//
// EACH GROUP'S SCRATCH IS SIZED BY THE WHOLE MESH TABLE AND NOT BY WHAT LANDS IN
// IT. Which group an entity is in is not known until the walk has finished, so
// the bound for each of them is every mesh there is; it is an arena, it is
// rewound at the end of the frame, and counting first would be a second walk to
// save memory that is given back a millisecond later.
//
// THE DEPTH CLEAR IS render'S CALL AND `3d` LEARNS NOTHING FROM MAKING IT. It
// takes no value: the number depth is cleared to lives in `render` beside the
// convention it belongs to, and this folder neither supplies it nor is told it.
// See render/include/render/device.h.
#include <3d/depth_sort.h>
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/normal_matrix.h>
#include <3d/projection.h>
#include <base/assert.h>
#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

// The world's one light, in the shape `render` takes it. The direction is
// already unit length — scene's light system is the only thing that writes one
// and it normalizes — so this is a copy of three fields and not arithmetic.
static voe_render_light the_sun(const voe_ecs_world *world)
{
	voe_scene_light light = voe_scene_light_rows(world)[0];
	voe_render_light sun = {
		.direction = light.direction,
		.intensity = light.intensity,
		.colour = light.colour,
	};

	return sun;
}

// One entity, held back until its group's turn: everything that group needs in
// order to issue the draw without looking anything up again.
struct deferred {
	voe_render_geometry geometry;
	voe_render_object object;
};

// One group of draws that could not be issued as the mesh table was walked,
// because it has to wait for a sort, for the depth clear, or for both.
//
// `depths` AND `order` ARE BOTH THERE OR BOTH ABSENT, AND THAT IS WHAT SAYS
// WHICH KIND OF GROUP THIS IS. A group with them sorts and draws blended; a
// group without them draws solid, in the order it was filled, which is table
// order. One field would do and two is what the sort already takes.
struct group {
	struct deferred *deferred;
	float *depths;
	uint32_t *order;
	uint32_t count;
};

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

// The record an entity is drawn with, which is the same two matrices and the
// same shading id whichever pass it ends up in.
static voe_render_object object_of(const voe_scene_transform *transform,
				   const voe_3d_material *material)
{
	voe_render_object object = { 0 };

	object.world = voe_scene_transform_matrix(*transform);
	// One inverse per drawn object per frame, which is the cost of getting a
	// non-uniformly scaled thing lit correctly. It is computed rather than
	// stored for the same reason the world matrix is
	// (scene/transform_component.h): a second copy of the truth is a thing
	// to invalidate. If it ever measures slow it becomes a cached column in
	// the transform table and nothing here changes.
	object.normal = voe_3d_normal_matrix(object.world);
	object.shading = material->shading.index;

	return object;
}

// Room in the arena for one group, sized for the whole mesh table — see the
// header for why that bound and not a measured one. A sorted group gets the two
// arrays the sort works in; an unsorted one has nothing to sort and gets neither.
//
// A TABLE WITH NOTHING IN IT PUSHES NOTHING. voe_base_arena_push asserts on a
// size of nought (base/arena.h), so an empty group is the zeroed struct and the
// fill and the draw below both do nothing with it.
static struct group group_new(voe_base_arena *arena, uint32_t capacity,
			      bool sorted)
{
	struct group group = { 0 };

	if (capacity == 0)
		return group;

	group.deferred = voe_base_arena_push(
		arena, (size_t)capacity * sizeof(*group.deferred));
	if (sorted) {
		group.depths = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.depths));
		group.order = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.order));
	}
	return group;
}

// Sets one entity aside in its group. The view matrix rather than a depth,
// because only a sorted group has anywhere to put a key — so the work of
// computing one is not done at all for a group drawn in the order it was filled.
static void hold(struct group *group, struct deferred entry,
		 voe_math_float4x4 view)
{
	group->deferred[group->count] = entry;
	if (group->depths != NULL)
		group->depths[group->count] =
			view_depth(view, entry.object.world);
	group->count++;
}

// One group's draws: sorted furthest away first through the blended pipeline, or
// in the order it was filled through the solid one. Returns false only when a
// draw was refused, which stops this group and not the frame — the same rule the
// draws issued during the walk follow.
static bool draw_group(voe_render_device *device, const struct group *group)
{
	bool sorted = group->order != NULL;

	if (sorted)
		voe_3d_depth_sort(group->depths, group->count, group->order);

	for (uint32_t i = 0; i < group->count; i++) {
		const struct deferred *drawn =
			&group->deferred[sorted ? group->order[i] : i];
		bool drawn_ok =
			sorted ? voe_render_frame_draw_blended(device,
							       drawn->geometry,
							       drawn->object)
			       : voe_render_frame_draw(device, drawn->geometry,
						       drawn->object);

		if (!drawn_ok)
			return false;
	}
	return true;
}

voe_3d_frame voe_3d_draw_system_frame(const voe_ecs_world *world,
				      voe_platform_size size)
{
	voe_3d_frame frame;
	voe_scene_camera camera;
	// A window with no area has no aspect ratio. One is as good as any
	// other then: _begin is about to say there is nothing to draw into and
	// nothing reads the matrix, so this only keeps the division below away
	// from a zero.
	float aspect = 1.0f;

	VOE_BASE_ASSERT(world != NULL, "framing no world");
	VOE_BASE_ASSERT(voe_scene_camera_count(world) == 1,
			"a world to draw needs exactly one camera — see 3d/draw_system.h");
	VOE_BASE_ASSERT(voe_scene_light_count(world) == 1,
			"a world to draw needs exactly one light — see 3d/draw_system.h");

	if (size.width > 0 && size.height > 0)
		aspect = (float)size.width / (float)size.height;

	camera = voe_scene_camera_rows(world)[0];
	frame.view.view = voe_scene_camera_view(camera);
	frame.view.projection = voe_3d_projection(camera, aspect);
	// Where the eye is, for the half of the shading that depends on which
	// direction a surface is being looked from. It is the camera's own
	// number and not something recovered from the view matrix.
	frame.view.eye = camera.eye;
	frame.view.reserved = 0.0f;
	frame.light = the_sun(world);

	return frame;
}

void voe_3d_draw_system_run(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, voe_3d_frame frame)
{
	voe_render_view view = frame.view;
	const voe_3d_mesh *meshes;
	const voe_ecs_entity *owners;
	uint32_t count;
	// The three groups this frame holds back, and the scratch they are built
	// in. The world's solid objects are the fourth and are drawn as they are
	// found, so they need none.
	struct voe_base_arena_mark mark;
	struct group world_blended = { 0 };
	struct group overlay_solid = { 0 };
	struct group overlay_blended = { 0 };

	VOE_BASE_ASSERT(world != NULL, "drawing no world");
	VOE_BASE_ASSERT(device != NULL, "drawing to no device");
	VOE_BASE_ASSERT(arena != NULL, "drawing with no arena to sort in");
	// The loop opens the frame and this draws into it; the same rule every
	// draw in render applies, asserted here once rather than found by the
	// first draw — or not found at all, in a world with nothing in it.
	VOE_BASE_DEBUG_ASSERT(voe_render_frame_is_open(device),
			      "drawing the world with no frame open — the loop calls voe_render_frame_begin first; see 3d/draw_system.h");

	meshes = voe_3d_mesh_rows(world);
	owners = voe_3d_mesh_entities(world);
	count = voe_3d_mesh_count(world);

	// The mark is taken here, immediately before the first push, so that
	// every path out above it has nothing to give back and the rewind at the
	// bottom is the only one.
	mark = voe_base_arena_mark(arena);

	world_blended = group_new(arena, count, true);
	overlay_solid = group_new(arena, count, false);
	overlay_blended = group_new(arena, count, true);

	for (uint32_t row = 0; row < count; row++) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, owners[row]);
		const voe_3d_material *material =
			voe_3d_material_get(world, owners[row]);
		struct deferred entry;
		bool blended;

		if (transform == NULL || material == NULL)
			continue;

		entry = (struct deferred){
			.geometry = meshes[row].geometry,
			.object = object_of(transform, material),
		};
		// Cutout is not blended and belongs with the solid ones — it
		// writes depth and needs no order.
		blended = material->alpha_mode == VOE_RENDER_ALPHA_BLENDED;

		// The two axes meet here and nowhere else: the layer says which
		// side of the depth clear this is on, the alpha mode says which
		// pass it is in on that side.
		if (meshes[row].layer == VOE_3D_LAYER_OVERLAY) {
			hold(blended ? &overlay_blended : &overlay_solid, entry,
			     view.view);
			continue;
		}
		// Set aside rather than drawn: it has to go after everything
		// solid in the world, and after every see-through thing further
		// away than it is.
		if (blended) {
			hold(&world_blended, entry, view.view);
			continue;
		}

		if (!voe_render_frame_draw(device, entry.geometry, entry.object))
			break;
	}

	// A refused draw in any group stops that group and not the frame, so
	// every return value here is deliberately dropped: the loop still ends
	// and presents the frame.
	(void)draw_group(device, &world_blended);

	// The world is finished and the overlay starts on an empty depth buffer,
	// which is the whole of what a layer is. The colour the world was drawn
	// in is untouched, so the overlay lands on top of that picture rather
	// than on a cleared one — and inside the overlay, depth works exactly as
	// it did in the world.
	//
	// AN EMPTY OVERLAY CLEARS NOTHING. A full-screen depth clear is real work
	// and a world with nothing above it should not pay for one; with both
	// groups empty the clear has nothing to make room for, and the depth
	// image is thrown away at the end of the frame either way.
	if (overlay_solid.count > 0 || overlay_blended.count > 0) {
		voe_render_frame_clear_depth(device);

		(void)draw_group(device, &overlay_solid);
		(void)draw_group(device, &overlay_blended);
	}

	// Everything above is this frame's, and the caller's arena is handed
	// back exactly as it arrived. Ending the frame is the loop's.
	voe_base_arena_rewind(arena, mark);
}
