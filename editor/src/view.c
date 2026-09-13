// The views: their cameras, their targets, the middle-button drag and the
// editor's sun. See the header for why the camera is not an entity, why the
// picture lags the layout by a frame, and why a view nobody shows is not drawn.
//
// THE ORBIT OWNS THE EYE. A view's yaw, pitch, focus and distance are what the
// drag changes, and the eye is always put back at the focus minus the distance
// along the direction the camera looks — so the camera looks at its focus by
// construction and nothing has to aim it.
#include "view.h"

#include <base/assert.h>

#include <3d/projection.h>

#include <math.h>

// The drag's rates, per millimetre the pointer travels on the surface. Radians
// for the orbit, metres for the pan and the dolly. Millimetres rather than
// pixels, so a drag across the same stretch of interface turns the camera as far
// on a small window as on a large one.
#define ORBIT_RADIANS_PER_MILLIMETRE 0.02f
#define PAN_METRES_PER_MILLIMETRE 0.02f
#define DOLLY_METRES_PER_MILLIMETRE 0.05f

// How close a dolly may bring the eye to the focus. Above nought, because an eye
// on its focus has no direction to look in.
#define CLOSEST 0.1f

// How far short of straight up or down the orbit stops, in radians. The view
// matrix builds its right-hand axis from world up, and at a pitch of a quarter
// turn that axis is nothing.
#define PITCH_LIMIT 1.55f

// What every view sees with. Radians and metres.
#define FIELD_OF_VIEW 0.9f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 200.0f

// The size a target is made at, before a panel has said how big it is. It is
// drawn at this for one frame and resized on the next.
#define FIRST_WIDTH 64u
#define FIRST_HEIGHT 64u

// THE EDITOR'S SUN: where its light goes, not where it is — down, and from the
// front-right, so the three faces a view from the front and above sees are three
// different brightnesses. Not unit length here; the_sun normalizes it once.
#define SUN_X (-0.4f)
#define SUN_Y (-1.0f)
#define SUN_Z (-0.6f)
#define SUN_INTENSITY 3.14159265f

// Where each view in use starts. View 0 from the front and above; view 1 from
// the side — +X, looking along −X — and a little above. Both look at the origin.
static const struct {
	float yaw;
	float pitch;
	float distance;
} first_cameras[VOE_EDITOR_VIEWS_IN_USE] = {
	{ 0.0f, -0.5f, 7.0f },
	{ 1.5707963f, -0.2f, 7.0f },
};

static voe_render_light the_sun(void)
{
	return (voe_render_light){
		.direction = voe_math_float3_normalize(
			(voe_math_float3){ SUN_X, SUN_Y, SUN_Z }),
		.intensity = SUN_INTENSITY,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
}

// The eye put back where the orbit says it is.
static void orbit_place(voe_editor_view *view)
{
	voe_math_float3 forward = voe_scene_camera_forward(view->camera);

	view->camera.eye = voe_math_float3_sub(
		view->focus, voe_math_float3_scale(forward, view->distance));
}

bool voe_editor_views_create(voe_editor_views *views, voe_render_device *gpu,
			     voe_base_error *error)
{
	VOE_BASE_ASSERT(views != NULL, "creating no views");
	VOE_BASE_ASSERT(gpu != NULL, "creating views on no device");

	*views = (voe_editor_views){ .count = VOE_EDITOR_VIEWS_IN_USE,
				     .captured = VOE_EDITOR_VIEW_NONE };

	for (uint32_t i = 0; i < VOE_EDITOR_VIEWS; i++)
		views->views[i].image = VOE_UI_NODE_NONE;

	for (uint32_t i = 0; i < views->count; i++) {
		voe_editor_view *view = &views->views[i];

		view->camera = (voe_scene_camera){
			.yaw = first_cameras[i].yaw,
			.pitch = first_cameras[i].pitch,
			.fov_y = FIELD_OF_VIEW,
			.near_plane = NEAR_PLANE,
			.far_plane = FAR_PLANE,
		};
		view->distance = first_cameras[i].distance;
		orbit_place(view);

		view->width = FIRST_WIDTH;
		view->height = FIRST_HEIGHT;
		if (!voe_render_target_create(gpu, view->width, view->height,
					      &view->target, &view->texture,
					      error))
			return false;
	}

	return true;
}

void voe_editor_view_fit(voe_editor_view *view, voe_render_device *gpu,
			 float pixels_per_millimetre)
{
	float wide;
	float high;
	uint32_t width;
	uint32_t height;

	VOE_BASE_ASSERT(view != NULL, "fitting no view");
	VOE_BASE_ASSERT(gpu != NULL, "fitting a view on no device");

	wide = roundf(view->rect.size.x * pixels_per_millimetre);
	high = roundf(view->rect.size.y * pixels_per_millimetre);
	// A rectangle that rounds to no pixels — nothing recorded yet, or a
	// panel squeezed to nothing — keeps the size the target has, because a
	// target of no area is not one `render` will make.
	if (wide < 1.0f || high < 1.0f)
		return;

	width = (uint32_t)wide;
	height = (uint32_t)high;
	if (width == view->width && height == view->height)
		return;

	voe_render_target_resize(gpu, view->target, width, height);
	view->width = width;
	view->height = height;
}

voe_render_pass_camera voe_editor_view_pass_camera(const voe_editor_view *view)
{
	VOE_BASE_ASSERT(view != NULL, "a pass camera for no view");
	VOE_BASE_ASSERT(view->width > 0 && view->height > 0,
			"a pass camera for a view with no size");

	return (voe_render_pass_camera){
		.view = { .view = voe_scene_camera_view(view->camera),
			  .projection = voe_3d_projection(
				  view->camera,
				  (float)view->width / (float)view->height),
			  .eye = view->camera.eye },
		.light = the_sun(),
	};
}

static bool contains(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.y >= rect.min.y &&
	       at.x < rect.min.x + rect.size.x &&
	       at.y < rect.min.y + rect.size.y;
}

// Which view a press at `at` starts over. Only a view whose picture was drawn
// last frame can be pressed on: anything else has no rectangle on the screen.
static uint32_t view_under(const voe_editor_views *views, voe_math_float2 at)
{
	for (uint32_t i = 0; i < views->count; i++) {
		if (views->views[i].image == VOE_UI_NODE_NONE)
			continue;
		if (contains(views->views[i].rect, at))
			return i;
	}

	return VOE_EDITOR_VIEW_NONE;
}

// THE SIGNS, which are the whole of how a drag feels. The pointer's y runs down.
// Orbit: dragging right swings the eye left round the focus, so the world turns
// with the hand; dragging down raises the eye. Pan: the world follows the hand,
// so the focus moves the other way. Dolly: dragging down pulls the eye back.
static void drag_view(voe_editor_view *view, voe_math_float2 travel,
		      bool shift, bool control)
{
	if (control) {
		view->distance += travel.y * DOLLY_METRES_PER_MILLIMETRE;
		if (view->distance < CLOSEST)
			view->distance = CLOSEST;
	} else if (shift) {
		voe_math_float3 forward =
			voe_scene_camera_forward(view->camera);
		voe_math_float3 right = { cosf(view->camera.yaw), 0.0f,
					  -sinf(view->camera.yaw) };
		voe_math_float3 up = voe_math_float3_cross(right, forward);

		view->focus = voe_math_float3_add(
			view->focus,
			voe_math_float3_scale(
				right, -travel.x * PAN_METRES_PER_MILLIMETRE));
		view->focus = voe_math_float3_add(
			view->focus,
			voe_math_float3_scale(
				up, travel.y * PAN_METRES_PER_MILLIMETRE));
	} else {
		view->camera.yaw -= travel.x * ORBIT_RADIANS_PER_MILLIMETRE;
		view->camera.pitch -= travel.y * ORBIT_RADIANS_PER_MILLIMETRE;
		if (view->camera.pitch > PITCH_LIMIT)
			view->camera.pitch = PITCH_LIMIT;
		if (view->camera.pitch < -PITCH_LIMIT)
			view->camera.pitch = -PITCH_LIMIT;
	}

	orbit_place(view);
}

void voe_editor_views_drag(voe_editor_views *views, voe_math_float2 pointer,
			   bool middle, bool shift, bool control)
{
	VOE_BASE_ASSERT(views != NULL, "dragging no views");

	if (middle && !views->middle_was_down)
		views->captured = view_under(views, pointer);
	else if (middle && views->captured != VOE_EDITOR_VIEW_NONE)
		drag_view(&views->views[views->captured],
			  voe_math_float2_sub(pointer, views->pointer), shift,
			  control);
	else if (!middle)
		views->captured = VOE_EDITOR_VIEW_NONE;

	views->middle_was_down = middle;
	views->pointer = pointer;
}

void voe_editor_views_images_clear(voe_editor_views *views)
{
	VOE_BASE_ASSERT(views != NULL, "clearing the pictures of no views");

	for (uint32_t i = 0; i < VOE_EDITOR_VIEWS; i++)
		views->views[i].image = VOE_UI_NODE_NONE;
}

void voe_editor_views_rects_read(voe_editor_views *views,
				 const voe_ui_context *ui)
{
	VOE_BASE_ASSERT(views != NULL, "reading the rectangles of no views");
	VOE_BASE_ASSERT(ui != NULL, "reading rectangles out of no interface");

	for (uint32_t i = 0; i < views->count; i++) {
		if (views->views[i].image == VOE_UI_NODE_NONE)
			continue;
		views->views[i].rect =
			voe_ui_node_rect(ui, views->views[i].image);
	}
}
