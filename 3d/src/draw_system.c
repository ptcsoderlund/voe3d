// The draw system: the camera, the sun, the matrices each object is drawn with,
// and one draw per mesh.
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
// A DRAW THAT IS REFUSED STOPS THE LOOP BUT NOT THE FRAME. Running out of room
// for objects means the device was made for fewer than this scene has; the
// frame is still ended and still presented, so a person sees most of the scene
// and a line on stderr rather than a black window. The alternative — abandoning
// the recording — would leave the slot's fence unsignalled.
//
// THE BLENDED PASS IS BUILT ON THE WAY THROUGH THE FIRST ONE AND NOT BY A SECOND
// WALK. A see-through entity's record and geometry are set aside as the mesh
// table is walked, together with the depth to sort on, so the two lookups it
// costs happen once. What is set aside is the record itself rather than the row,
// because the second pass would otherwise look the same two components up again.
//
// THE SCRATCH IS SIZED BY THE WHOLE MESH TABLE AND NOT BY WHAT TURNS OUT TO BE
// BLENDED. How many are see-through is not known until the walk has finished, so
// the bound is every mesh there is; it is an arena, it is rewound at the end of
// the frame, and counting first would be a second walk to save memory that is
// given back a millisecond later.
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

// One see-through entity, held between the two passes: everything the second
// pass needs in order to issue the draw without looking anything up again.
struct deferred {
	voe_render_geometry geometry;
	voe_render_object object;
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

// The blended pass: the sort, then the draws, furthest away first. Returns false
// only when a draw was refused, which stops this pass and not the frame — the
// same rule the solid pass follows.
static bool draw_blended(voe_render_device *device,
			 const struct deferred *deferred, const float *depths,
			 uint32_t *order, uint32_t count)
{
	voe_3d_depth_sort(depths, count, order);

	for (uint32_t i = 0; i < count; i++) {
		const struct deferred *drawn = &deferred[order[i]];

		if (!voe_render_frame_draw_blended(device, drawn->geometry,
						   drawn->object))
			return false;
	}
	return true;
}

bool voe_3d_draw_system_run(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, voe_platform_size size)
{
	voe_render_view view;
	voe_scene_camera camera;
	const voe_3d_mesh *meshes;
	const voe_ecs_entity *owners;
	uint32_t count;
	bool drawing = false;
	// This frame's blended pass, and the scratch it is built in.
	struct voe_base_arena_mark mark;
	struct deferred *deferred = NULL;
	float *depths = NULL;
	uint32_t *order = NULL;
	uint32_t blended = 0;

	VOE_BASE_ASSERT(world != NULL, "drawing no world");
	VOE_BASE_ASSERT(device != NULL, "drawing to no device");
	VOE_BASE_ASSERT(arena != NULL, "drawing with no arena to sort in");
	VOE_BASE_ASSERT(voe_scene_camera_count(world) == 1,
			"a world to draw needs exactly one camera — see 3d/draw_system.h");
	VOE_BASE_ASSERT(voe_scene_light_count(world) == 1,
			"a world to draw needs exactly one light — see 3d/draw_system.h");

	// A window with no area has no aspect ratio to compute, and render is
	// going to say there is nothing to draw into anyway. Returning here
	// keeps the division below out of reach of a zero.
	if (size.width <= 0 || size.height <= 0)
		return true;

	camera = voe_scene_camera_rows(world)[0];
	view.view = voe_scene_camera_view(camera);
	view.projection = voe_3d_projection(camera,
					    (float)size.width /
						    (float)size.height);
	// Where the eye is, for the half of the shading that depends on which
	// direction a surface is being looked from. It is the camera's own
	// number and not something recovered from the view matrix.
	view.eye = camera.eye;

	if (!voe_render_frame_begin(device, size, view, the_sun(world),
				    &drawing))
		return false;
	if (!drawing)
		return true;

	meshes = voe_3d_mesh_rows(world);
	owners = voe_3d_mesh_entities(world);
	count = voe_3d_mesh_count(world);

	// The mark is taken here, immediately before the first push, so that
	// every path out above it has nothing to give back and the rewind at the
	// bottom is the only one.
	mark = voe_base_arena_mark(arena);

	// Nothing to draw and therefore nothing to sort. The push below asserts
	// on a size of nought (base/arena.h), so this is the branch that keeps
	// it out of reach rather than a shortcut.
	if (count > 0) {
		deferred = voe_base_arena_push(arena,
					       (size_t)count * sizeof(*deferred));
		depths = voe_base_arena_push(arena,
					     (size_t)count * sizeof(*depths));
		order = voe_base_arena_push(arena,
					    (size_t)count * sizeof(*order));
	}

	for (uint32_t row = 0; row < count; row++) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, owners[row]);
		const voe_3d_material *material =
			voe_3d_material_get(world, owners[row]);
		voe_render_object object;

		if (transform == NULL || material == NULL)
			continue;

		object = object_of(transform, material);

		// Set aside rather than drawn: it has to go after everything
		// solid, and after every see-through thing further away than it
		// is. Cutout is not blended and belongs in this pass — it writes
		// depth and needs no order.
		if (material->alpha_mode == VOE_RENDER_ALPHA_BLENDED) {
			deferred[blended] = (struct deferred){
				.geometry = meshes[row].geometry,
				.object = object,
			};
			depths[blended] = view_depth(view.view, object.world);
			blended++;
			continue;
		}

		if (!voe_render_frame_draw(device, meshes[row].geometry, object))
			break;
	}

	// A refused draw in either pass stops that pass and not the frame, so
	// the return value is deliberately dropped here: _end still runs.
	(void)draw_blended(device, deferred, depths, order, blended);

	// Everything above is this frame's, and the caller's arena is handed
	// back exactly as it arrived.
	voe_base_arena_rewind(arena, mark);
	return voe_render_frame_end(device);
}
