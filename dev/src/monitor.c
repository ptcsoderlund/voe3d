// The monitor's target, camera, screen and frame. See the header for why the
// camera is not an entity, why the frame hides the screen and why the screen is
// one-sided and unlit.
//
// THE SCREEN'S FOUR VERTICES ARE src/quad.c's +Z FACE, corner for corner and
// index for index, with the second face left off. Copying the pair that is
// already proven counter-clockwise-from-outside is what keeps a hand-wound face
// from being wound the wrong way and silently culled — and a silently culled
// screen looks exactly like a target nothing drew into.
//
// AND ITS TEXTURE COORDINATES PUT (0, 0) AT THE TOP-LEFT, which is what `render`
// and the target's picture both mean by it. A screen showing its picture upside
// down or mirrored is this array and nothing else: nothing between here and the
// card turns a picture over (assets/include/assets/image.h).
#include "monitor.h"

#include "motion.h"

#include <base/assert.h>

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/projection.h>

#include <math/float2.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/quat.h>

#include <scene/transform_system.h>

#define VOE_DEV_MONITOR_VERTEX_COUNT 4
#define VOE_DEV_MONITOR_INDEX_COUNT 6

// Half a metre, so the screen's geometry is one metre across and the transform's
// scale below is its size in metres. Units are metres (CLAUDE.md).
#define H 0.5f

// Where the second camera stands, what it can see, and which way it looks. It
// stands above the scene and inside the orbit, looking down at the cubes: the
// world's own camera flies a circle of radius ORBIT_RADIUS at ORBIT_HEIGHT and
// never comes here, so the two pictures are never the same picture and the one
// on the screen holds still while the one around it swings. Zero yaw looks along
// -Z and a negative pitch looks down (src/motion.h).
//
// AND IT STANDS CLOSE ENOUGH THAT THE WORLD FILLS THE PICTURE. A second camera
// out where the orbit is leaves most of a 512-pixel square showing the clear
// colour, which is the same colour the window is cleared to — so the screen
// would have no visible edge and would read as a hole rather than as a picture.
#define EYE_X 0.0f
#define EYE_Y 4.5f
#define EYE_Z 5.0f
#define YAW 0.0f
#define PITCH (-0.62f)
#define FIELD_OF_VIEW 1.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// Where the screen stands, how big it is and which way it faces. It is off to
// the left, where the scene has room since the lettered test model left, and far
// enough back in z to stand clear of the pair of sprites that lives out there.
//
// IT IS TURNED TOWARDS THE MIDDLE OF THE SCENE, which is what makes it a screen
// somebody put there rather than a square that happens to be flat on. A rotation
// about +Y takes the quad's +Z normal to (sin, 0, cos), so a positive angle
// turns its face towards +X — towards the cubes, and towards the half of the
// orbit that has the most to look at.
#define SCREEN_X (-4.2f)
#define SCREEN_Y 1.2f
#define SCREEN_Z (-0.6f)
#define SCREEN_SIZE 2.4f
#define SCREEN_YAW 0.6f

// One face, wound counter-clockwise seen from +Z, normal +Z, (0, 0) at the
// top-left. See the header of this file.
static const voe_render_vertex screen_vertices[VOE_DEV_MONITOR_VERTEX_COUNT] = {
	{ { -H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
};

static const uint32_t screen_indices[VOE_DEV_MONITOR_INDEX_COUNT] = {
	3, 2, 1, 3, 1, 0,
};

// The material the screen wears: the target's texture, nothing else on it, and
// unlit so what shows is the picture rather than the picture times a lambert
// term. Opaque, because a picture of a whole frame has nothing behind it to let
// through. The UV rectangle is the whole texture, said rather than left zeroed —
// voe_3d_material_upload would write the same answer back, and a material that
// reads its whole picture is worth seeing spelled out beside the texture it
// reads.
static voe_3d_material screen_material(voe_render_texture texture)
{
	return (voe_3d_material){
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.unlit = true,
		.base_colour_texture = texture,
		.base_colour_uv_offset = { 0.0f, 0.0f },
		.base_colour_uv_scale = { 1.0f, 1.0f },
	};
}

bool voe_dev_monitor_create(voe_dev_monitor *out, voe_ecs_world *world,
			    voe_render_device *gpu, voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_3d_material material;
	voe_scene_transform transform = {
		.position = { SCREEN_X, SCREEN_Y, SCREEN_Z },
		.scale = { SCREEN_SIZE, SCREEN_SIZE, 1.0f },
	};

	VOE_BASE_ASSERT(out != NULL, "no monitor to build");
	VOE_BASE_ASSERT(world != NULL, "a monitor in no world");
	VOE_BASE_ASSERT(gpu != NULL, "a monitor on no device");

	*out = (voe_dev_monitor){
		.pose = voe_dev_flight_pose((voe_dev_flight){
			.eye = { EYE_X, EYE_Y, EYE_Z },
			.yaw = YAW,
			.pitch = PITCH }),
		.lens = { .fov_y = FIELD_OF_VIEW,
			  .near_plane = NEAR_PLANE,
			  .far_plane = FAR_PLANE },
	};

	transform.rotation = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 1.0f, 0.0f }, SCREEN_YAW);

	// The target and the texture that shows it, in one call and once. It
	// waits for the card to go idle, which is why this is startup work and
	// not something the loop does.
	if (!voe_render_target_create(gpu, VOE_DEV_MONITOR_PIXELS,
				      VOE_DEV_MONITOR_PIXELS, &out->target,
				      &out->texture, error))
		return false;

	if (!voe_render_geometry_create(gpu, screen_vertices,
					VOE_DEV_MONITOR_VERTEX_COUNT,
					screen_indices,
					VOE_DEV_MONITOR_INDEX_COUNT, &geometry,
					error))
		return false;

	material = screen_material(out->texture);
	if (!voe_3d_material_upload(gpu, &material, error))
		return false;

	if (!voe_ecs_entity_create(world, &out->screen))
		return false;
	if (!voe_scene_transform_add(world, out->screen, transform))
		return false;
	if (!voe_3d_mesh_add(world, out->screen,
			     (voe_3d_mesh){ .geometry = geometry,
					    .layer = VOE_3D_LAYER_WORLD }))
		return false;
	return voe_3d_material_add(world, out->screen, material);
}

voe_3d_frame voe_dev_monitor_frame(const voe_dev_monitor *monitor,
				   const voe_ecs_world *world)
{
	voe_3d_frame frame;

	VOE_BASE_ASSERT(monitor != NULL, "a frame for no monitor");
	VOE_BASE_ASSERT(world != NULL, "a monitor frame with no world");

	frame = (voe_3d_frame){
		.light = voe_3d_draw_system_light(world),
		// Not optional: the screen wears this target's texture, so the
		// pass that fills the target must not draw it. ADR-0158, and
		// the header says what the debug check says when this is
		// forgotten.
		.hidden = monitor->screen,
		// What the view and every matrix are taken about (ADR-0250),
		// and drawn as it is now: dev steps nothing (ADR-0254).
		.eye = monitor->pose.position,
		.lag = 0.0f,
	};

	// The aspect ratio is the target's own and not the window's: this
	// picture is square whatever shape the window is dragged into, which is
	// what keeps the screen from stretching when nothing about it moved.
	// A pose of scale one always has an inverse, so blind never happens;
	// it is set rather than assumed because the answer is there to read.
	frame.blind = !voe_3d_view(monitor->pose, monitor->lens, 1.0f,
				   &frame.view);
	return frame;
}
