// The views: their cameras, their targets and the preview's, the middle-button drag, and the
// light, the outline colour and the gizmo's colour a view is drawn with. See
// the header for why the camera is not an entity, why the light is the world's and not the views' to
// hold, why the picture lags the layout by a frame, and why a view nobody shows
// is not drawn.
//
// THE ORBIT OWNS THE EYE. A view's yaw, pitch, focus and distance are what the
// drag changes, and the eye is always put back at the focus minus the distance
// along the direction the camera looks — so the camera looks at its focus by
// construction and nothing has to aim it. That still holds while a view flies:
// the fly turns about the eye by moving the focus to stay `distance` ahead, and
// moves the eye and the focus together, then puts the eye back as the orbit does.
#include "view.h"

#include <base/assert.h>

#include <3d/draw_system.h>
#include <3d/projection.h>

#include <math/quat.h>

#include <scene/transform_component.h>

#include <math.h>

// The drag's rates, per millimetre the pointer travels on the surface. Radians
// for the orbit, metres for the pan and the dolly. Millimetres rather than
// pixels, so a drag across the same stretch of interface turns the camera as far
// on a small window as on a large one.
#define ORBIT_RADIANS_PER_MILLIMETRE 0.02f
#define PAN_METRES_PER_MILLIMETRE 0.02f
#define DOLLY_METRES_PER_MILLIMETRE 0.05f

// The fly's rates (0233): radians per unit of the platform's pointer motion,
// metres a second, and how many times that while `fast` is held.
#define FLY_RADIANS_PER_UNIT 0.004f
#define FLY_METRES_PER_SECOND 4.0f
#define FLY_FAST 3.0f

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

// The way the view looks: zero yaw down −Z, positive yaw towards −X, positive
// pitch up — the forward of the pose voe_editor_view_pass_camera builds.
static voe_math_float3 orbit_forward(const voe_editor_view *view)
{
	float level = cosf(view->pitch);

	return (voe_math_float3){ -sinf(view->yaw) * level, sinf(view->pitch),
				  -cosf(view->yaw) * level };
}

// A world position moved by a float step, the sum in double.
static voe_math_double3 moved(voe_math_double3 at, voe_math_float3 by)
{
	return voe_math_double3_add(at, voe_math_double3_from_float3(by));
}

// The eye put back where the orbit says it is.
static void orbit_place(voe_editor_view *view)
{
	voe_math_float3 forward = orbit_forward(view);

	view->eye = moved(view->focus,
			  voe_math_float3_scale(forward, -view->distance));
}

bool voe_editor_views_create(voe_editor_views *views, voe_render_device *gpu,
			     voe_base_error *error)
{
	VOE_BASE_ASSERT(views != NULL, "creating no views");
	VOE_BASE_ASSERT(gpu != NULL, "creating views on no device");

	*views = (voe_editor_views){ .count = VOE_EDITOR_VIEWS_IN_USE,
				     .captured = VOE_EDITOR_VIEW_NONE,
				     .flying = VOE_EDITOR_VIEW_NONE };

	for (uint32_t i = 0; i < VOE_EDITOR_VIEWS; i++)
		views->views[i].image = VOE_UI_NODE_NONE;

	for (uint32_t i = 0; i < views->count; i++) {
		voe_editor_view *view = &views->views[i];

		view->yaw = first_cameras[i].yaw;
		view->pitch = first_cameras[i].pitch;
		view->lens = (voe_scene_camera){
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

	return voe_render_target_create(gpu, VOE_EDITOR_PREVIEW_WIDTH,
					VOE_EDITOR_PREVIEW_HEIGHT,
					&views->preview_target,
					&views->preview_texture, error);
}

void voe_editor_views_focus_camera(voe_editor_views *views,
				   const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(views != NULL, "focusing no views");
	VOE_BASE_ASSERT(world != NULL, "focusing views on no world");

	voe_math_double3 focus = { 0 };
	if (voe_scene_camera_count(world) > 0) {
		voe_ecs_entity camera = voe_scene_camera_entities(world)[0];
		if (voe_scene_transform_get(world, camera) != NULL)
			focus = voe_scene_transform_world(world, camera).position;
	}

	for (uint32_t i = 0; i < views->count; i++) {
		views->views[i].focus = focus;
		orbit_place(&views->views[i]);
	}
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

voe_render_light voe_editor_view_light(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "the light of no world");

	return voe_3d_draw_system_light(world);
}

voe_math_float3 voe_editor_view_outline_colour(const voe_ui_theme *palette)
{
	VOE_BASE_ASSERT(palette != NULL, "an outline colour out of no palette");

	voe_math_float4 fill = palette->inverse;
	voe_math_float4 ink = palette->inverse_ink;
	float fill_luminance =
		0.2126f * fill.x + 0.7152f * fill.y + 0.0722f * fill.z;
	float ink_luminance =
		0.2126f * ink.x + 0.7152f * ink.y + 0.0722f * ink.z;
	voe_math_float4 lighter = fill_luminance >= ink_luminance ? fill : ink;

	return (voe_math_float3){ lighter.x, lighter.y, lighter.z };
}

// What a handle at rest is dimmed to: a bit over half the marked colour's
// linear value, which reads as clearly darker than the marked handle beside it
// while staying well above the near-black ground a view is cleared to.
#define VOE_EDITOR_VIEW_GIZMO_REST 0.55f

voe_math_float3 voe_editor_view_gizmo_colour(const voe_ui_theme *palette,
					     bool marked)
{
	VOE_BASE_ASSERT(palette != NULL, "a gizmo colour out of no palette");

	voe_math_float3 colour = voe_editor_view_outline_colour(palette);
	return marked ? colour :
			voe_math_float3_scale(colour,
					      VOE_EDITOR_VIEW_GIZMO_REST);
}

voe_render_pass_camera voe_editor_view_pass_camera(const voe_editor_view *view,
						   voe_render_light light)
{
	VOE_BASE_ASSERT(view != NULL, "a pass camera for no view");
	VOE_BASE_ASSERT(view->width > 0 && view->height > 0,
			"a pass camera for a view with no size");

	// Yaw about +Y and then pitch about the turned X: _mul reads right to
	// left, so the pitch is applied first, about the eye's own X (0223).
	voe_scene_transform pose = {
		.position = view->eye,
		.rotation = voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f }, view->yaw),
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 1.0f, 0.0f, 0.0f },
				view->pitch)),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_pass_camera camera = { .light = light };
	bool sees = voe_3d_view(pose, view->lens,
				(float)view->width / (float)view->height,
				&camera.view);

	VOE_BASE_ASSERT(sees, "an orbit whose pose sees nothing");
	return camera;
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
		voe_math_float3 forward = orbit_forward(view);
		voe_math_float3 right = { cosf(view->yaw), 0.0f,
					  -sinf(view->yaw) };
		voe_math_float3 up = voe_math_float3_cross(right, forward);

		view->focus = moved(
			view->focus,
			voe_math_float3_add(
				voe_math_float3_scale(
					right,
					-travel.x * PAN_METRES_PER_MILLIMETRE),
				voe_math_float3_scale(
					up, travel.y * PAN_METRES_PER_MILLIMETRE)));
	} else {
		view->yaw -= travel.x * ORBIT_RADIANS_PER_MILLIMETRE;
		view->pitch -= travel.y * ORBIT_RADIANS_PER_MILLIMETRE;
		if (view->pitch > PITCH_LIMIT)
			view->pitch = PITCH_LIMIT;
		if (view->pitch < -PITCH_LIMIT)
			view->pitch = -PITCH_LIMIT;
	}

	orbit_place(view);
}

void voe_editor_views_drag(voe_editor_views *views, voe_math_float2 pointer,
			   bool middle, bool shift, bool control)
{
	VOE_BASE_ASSERT(views != NULL, "dragging no views");

	if (middle && !views->middle_was_down)
		views->captured = views->flying == VOE_EDITOR_VIEW_NONE ?
					  view_under(views, pointer) :
					  VOE_EDITOR_VIEW_NONE;
	else if (middle && views->captured != VOE_EDITOR_VIEW_NONE)
		drag_view(&views->views[views->captured],
			  voe_math_float2_sub(pointer, views->pointer), shift,
			  control);
	else if (!middle)
		views->captured = VOE_EDITOR_VIEW_NONE;

	views->middle_was_down = middle;
	views->pointer = pointer;
}

// THE SIGNS of the fly. Moving the pointer right turns the view right, which is
// yaw falling; moving it down looks down, which is pitch falling. The eye stays
// and the focus is put `distance` ahead of it along the new look.
static void fly_view(voe_editor_view *view, voe_math_float2 turn,
		     voe_editor_fly_keys keys, float seconds)
{
	view->yaw -= turn.x * FLY_RADIANS_PER_UNIT;
	view->pitch -= turn.y * FLY_RADIANS_PER_UNIT;
	if (view->pitch > PITCH_LIMIT)
		view->pitch = PITCH_LIMIT;
	if (view->pitch < -PITCH_LIMIT)
		view->pitch = -PITCH_LIMIT;

	voe_math_float3 forward = orbit_forward(view);
	voe_math_float3 right = { cosf(view->yaw), 0.0f, -sinf(view->yaw) };
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 way = { 0.0f, 0.0f, 0.0f };

	view->focus =
		moved(view->eye, voe_math_float3_scale(forward, view->distance));

	way = voe_math_float3_add(
		way, voe_math_float3_scale(forward, (float)keys.forward -
							    (float)keys.back));
	way = voe_math_float3_add(
		way, voe_math_float3_scale(right, (float)keys.right -
							  (float)keys.left));
	way = voe_math_float3_add(
		way, voe_math_float3_scale(up, (float)keys.up - (float)keys.down));

	if (voe_math_float3_length(way) > 0.0f) {
		voe_math_float3 step = voe_math_float3_scale(
			voe_math_float3_normalize(way),
			FLY_METRES_PER_SECOND * (keys.fast ? FLY_FAST : 1.0f) *
				seconds);

		view->eye = moved(view->eye, step);
		view->focus = moved(view->focus, step);
	}

	orbit_place(view);
}

bool voe_editor_views_fly(voe_editor_views *views, voe_math_float2 pointer,
			  bool right, voe_math_float2 turn,
			  voe_editor_fly_keys keys, float seconds)
{
	VOE_BASE_ASSERT(views != NULL, "flying no views");
	VOE_BASE_ASSERT(seconds >= 0.0f, "flying for a time before now");

	if (right && !views->right_was_down)
		views->flying = views->captured == VOE_EDITOR_VIEW_NONE ?
					view_under(views, pointer) :
					VOE_EDITOR_VIEW_NONE;
	else if (!right)
		views->flying = VOE_EDITOR_VIEW_NONE;

	if (views->flying != VOE_EDITOR_VIEW_NONE)
		fly_view(&views->views[views->flying], turn, keys, seconds);

	views->right_was_down = right;
	VOE_BASE_ASSERT(views->flying == VOE_EDITOR_VIEW_NONE ||
				views->flying < views->count,
			"flying a view that is not in use");
	return views->flying != VOE_EDITOR_VIEW_NONE;
}

bool voe_editor_views_under(const voe_editor_views *views,
			    voe_math_float2 pointer, uint32_t *view,
			    voe_math_float2 *point)
{
	VOE_BASE_ASSERT(views != NULL, "asking no views what is under a pointer");
	VOE_BASE_ASSERT(view != NULL, "nowhere to put the view a pointer is over");
	VOE_BASE_ASSERT(point != NULL, "nowhere to put the place in the picture");

	uint32_t found = view_under(views, pointer);
	if (found == VOE_EDITOR_VIEW_NONE)
		return false;

	// The picture is drawn `width` by `height` pixels and shown stretched
	// over the whole rectangle, so the fraction across the rectangle is the
	// fraction across the picture. contains() answered true, so the
	// rectangle has room in both directions and neither division is by
	// nought.
	const voe_editor_view *over = &views->views[found];
	*view = found;
	*point = (voe_math_float2){
		.x = (pointer.x - over->rect.min.x) / over->rect.size.x *
		     (float)over->width,
		.y = (pointer.y - over->rect.min.y) / over->rect.size.y *
		     (float)over->height,
	};

	return true;
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
