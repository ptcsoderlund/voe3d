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

bool voe_3d_draw_system_run(voe_ecs_world *world, voe_render_device *device,
			    voe_platform_size size)
{
	voe_render_view view;
	voe_scene_camera camera;
	const voe_3d_mesh *meshes;
	const voe_ecs_entity *owners;
	uint32_t count;
	bool drawing = false;

	VOE_BASE_ASSERT(world != NULL, "drawing no world");
	VOE_BASE_ASSERT(device != NULL, "drawing to no device");
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

	for (uint32_t row = 0; row < count; row++) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, owners[row]);
		const voe_3d_material *material =
			voe_3d_material_get(world, owners[row]);
		voe_render_object object = { 0 };

		if (transform == NULL || material == NULL)
			continue;

		object.world = voe_scene_transform_matrix(*transform);
		// One inverse per drawn object per frame, which is the cost of
		// getting a non-uniformly scaled thing lit correctly. It is
		// computed rather than stored for the same reason the world
		// matrix is (scene/transform_component.h): a second copy of the
		// truth is a thing to invalidate. If it ever measures slow it
		// becomes a cached column in the transform table and nothing
		// here changes.
		object.normal = voe_3d_normal_matrix(object.world);
		object.shading = material->shading.index;

		if (!voe_render_frame_draw(device, meshes[row].geometry, object))
			break;
	}

	return voe_render_frame_end(device);
}
