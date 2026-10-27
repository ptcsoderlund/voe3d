// The hover, the press that grabs a handle, a ring or a scale handle and the
// move, turn or scale that follows it, each against a gizmo built from the
// view's own camera and picture size. See the header for why the middle button
// is not read here, why the drag is measured from the press, and why a whole
// transform, relative to the parent, is submitted.
#include "gizmo.h"

#include <3d/gizmo_rings.h>
#include <3d/gizmo_scales.h>
#include <3d/pick.h>

#include <base/assert.h>
#include <base/report.h>

#include <scene/transform_component.h>
#include <scene/transform_system.h>

// The ray through `point` of `view`'s picture and the gizmo standing at
// `origin` in it, the camera built exactly as the view's pass is. The light is
// nothing to a gizmo, so the pass camera is asked with none.
static voe_3d_ray gizmo_in_view(const voe_editor_view *view,
				voe_math_float2 point, float pixels,
				voe_math_double3 origin, voe_3d_gizmo *gizmo)
{
	voe_platform_size size = { (int)view->width, (int)view->height };

	voe_render_view seen =
		voe_editor_view_pass_camera(view, (voe_render_light){ 0 }).view;

	*gizmo = voe_3d_gizmo_at(origin, seen, view->eye, size, pixels);
	return voe_3d_pick_ray(seen, view->eye, size, point);
}

// The world axis a ring turns about: X, Y or Z, the only handles a ring is.
static voe_math_float3 gizmo_ring_axis(voe_3d_gizmo_handle handle)
{
	return (voe_math_float3){ handle == VOE_3D_GIZMO_X ? 1.0f : 0.0f,
				  handle == VOE_3D_GIZMO_Y ? 1.0f : 0.0f,
				  handle == VOE_3D_GIZMO_Z ? 1.0f : 0.0f };
}

// `placed`'s rotation replaced by the kept one turned about the held ring's world
// axis by the angle swept since the press (0274), pre-multiplied so the turn is
// the world's and not the entity's own, and normalized so a long drag hands the
// drain a unit rotation. False, `placed` untouched, for a refused angle (the last
// submitted stands) or a rotation that has not changed.
static bool gizmo_turn(const voe_editor_gizmo *gizmo, voe_3d_gizmo at,
		       voe_3d_ray ray, voe_scene_transform *placed)
{
	float angle;

	if (!voe_3d_gizmo_rings_angle(at, gizmo->held, ray, &angle))
		return false;

	voe_math_quat rotation = voe_math_quat_normalize(voe_math_quat_mul(
		voe_math_quat_from_axis_angle(gizmo_ring_axis(gizmo->held),
					      angle - gizmo->grab_angle),
		gizmo->kept));
	if (rotation.x == placed->rotation.x &&
	    rotation.y == placed->rotation.y &&
	    rotation.z == placed->rotation.z &&
	    rotation.w == placed->rotation.w)
		return false;
	placed->rotation = rotation;
	return true;
}

// `placed`'s scale replaced by the world scale at the press times the factor
// the drag has come to (0482 point 2), on the held axis, the entity's own, or on
// all three for the centre; the gizmo turned by `kept`, as it stood at the
// press. False, `placed` untouched, for a refused grab (the last submitted
// stands) or a scale that has not changed.
static bool gizmo_scale(const voe_editor_gizmo *gizmo, voe_3d_gizmo at,
			voe_3d_ray ray, voe_scene_transform *placed)
{
	voe_math_double3 now;

	VOE_BASE_ASSERT(placed != NULL, "scaling no transform");
	VOE_BASE_ASSERT(gizmo->held != VOE_3D_GIZMO_NONE, "scaling by no handle");

	if (!voe_3d_gizmo_scales_grab(at, gizmo->kept, gizmo->held, ray, &now))
		return false;

	float factor = voe_3d_gizmo_scales_factor(at, gizmo->kept, gizmo->held,
						  gizmo->grab, now);
	bool all = gizmo->held == VOE_3D_GIZMO_CENTRE;
	voe_math_float3 scale = voe_math_float3_mul(
		gizmo->scale_start,
		(voe_math_float3){
			all || gizmo->held == VOE_3D_GIZMO_X ? factor : 1.0f,
			all || gizmo->held == VOE_3D_GIZMO_Y ? factor : 1.0f,
			all || gizmo->held == VOE_3D_GIZMO_Z ? factor : 1.0f });
	if (scale.x == placed->scale.x && scale.y == placed->scale.y &&
	    scale.z == placed->scale.z)
		return false;
	placed->scale = scale;
	return true;
}

// One frame of a running drag in the captured view, wherever the pointer is
// now. The point is worked out as voe_editor_views_under does, but for this view
// even once the pointer has left it, as the views' own drag goes on. `placed`
// is the entity's world transform; the row submitted is it made relative to the
// entity's parent.
static void gizmo_drag(voe_editor_gizmo *gizmo, voe_ecs_world *world,
		       voe_ecs_entity entity, voe_scene_transform placed,
		       const voe_editor_view *view, float pixels,
		       voe_math_float2 pointer)
{
	voe_3d_gizmo at;
	voe_math_double3 now;

	// A rectangle of no size is a view that was never shown; there is no
	// picture to measure against, so the entity stays where it is.
	if (view->rect.size.x <= 0.0f || view->rect.size.y <= 0.0f)
		return;

	voe_math_float2 point = {
		.x = (pointer.x - view->rect.min.x) / view->rect.size.x *
		     (float)view->width,
		.y = (pointer.y - view->rect.min.y) / view->rect.size.y *
		     (float)view->height,
	};
	voe_3d_ray ray = gizmo_in_view(view, point, pixels, gizmo->start, &at);

	if (gizmo->kind == VOE_3D_GIZMO_RINGS) {
		if (!gizmo_turn(gizmo, at, ray, &placed))
			return;
	} else if (gizmo->kind == VOE_3D_GIZMO_SCALES) {
		if (!gizmo_scale(gizmo, at, ray, &placed))
			return;
	} else {
		// A refused grab is a glancing ray this frame (3d/gizmo.h): the
		// entity keeps the position it had.
		if (!voe_3d_gizmo_grab(at, gizmo->held, ray, &now))
			return;

		voe_math_double3 position = voe_math_double3_add(
			gizmo->start, voe_math_double3_sub(now, gizmo->grab));
		if (position.x == placed.position.x &&
		    position.y == placed.position.y &&
		    position.z == placed.position.z)
			return;
		placed.position = position;
	}

	voe_scene_transform row =
		voe_scene_transform_local(world, entity, placed);
	if (!voe_scene_transform_submit(
		    world, (voe_scene_transform_intent){ .entity = entity,
							 .transform = row })) {
		VOE_BASE_WARNING("editor",
				 "the transform queue is full; that move was dropped");
		return;
	}
	gizmo->moved++;
}

void voe_editor_gizmo_read(voe_editor_gizmo *gizmo, voe_editor_scene *scene,
			   const voe_editor_views *views, float pixels,
			   voe_math_float2 pointer, bool down, bool blocked)
{
	uint32_t view;
	voe_math_float2 point;
	voe_3d_gizmo at;
	voe_math_double3 grab = { 0 }; // a turn grabs an angle and no point

	VOE_BASE_ASSERT(gizmo != NULL, "reading a press into no gizmo");
	VOE_BASE_ASSERT(scene != NULL, "a gizmo in no scene");
	VOE_BASE_ASSERT(views != NULL, "a gizmo in no views");
	VOE_BASE_ASSERT(pixels > 0.0f, "a gizmo of no size on the picture");

	bool pressed = down && !gizmo->pointer_was_down;
	gizmo->pointer_was_down = down;
	gizmo->moved = 0;
	gizmo->hovered = VOE_3D_GIZMO_NONE;
	gizmo->hovered_view = VOE_EDITOR_VIEW_NONE;

	// A prefab's part is its prefab's to place (0283 point 5): no gizmo.
	const voe_scene_transform *row =
		scene->world == NULL ||
				voe_editor_inspector_is_part(
					scene->world, scene->selected, NULL) ?
			NULL :
			voe_scene_transform_get(scene->world, scene->selected);
	if (blocked || row == NULL || !down || gizmo->kind != scene->gizmo_kind)
		gizmo->held = VOE_3D_GIZMO_NONE;
	if (blocked || row == NULL)
		return;
	voe_scene_transform placed =
		voe_scene_transform_world(scene->world, scene->selected);

	if (gizmo->held != VOE_3D_GIZMO_NONE) {
		VOE_BASE_ASSERT(gizmo->captured < views->count,
				"a drag captured in no view");
		gizmo_drag(gizmo, scene->world, scene->selected, placed,
			   &views->views[gizmo->captured], pixels, pointer);
		return;
	}

	if (!voe_editor_views_under(views, pointer, &view, &point))
		return;

	// The scales stand on the entity's own axes, its world rotation the turn.
	voe_3d_gizmo_kind kind = scene->gizmo_kind;
	voe_3d_ray ray = gizmo_in_view(&views->views[view], point, pixels,
				       placed.position, &at);
	voe_3d_gizmo_handle under =
		kind == VOE_3D_GIZMO_RINGS ?
			voe_3d_gizmo_rings_hit(at, ray) :
		kind == VOE_3D_GIZMO_SCALES ?
			voe_3d_gizmo_scales_hit(at, placed.rotation, ray) :
			voe_3d_gizmo_hit(at, ray);
	if (under == VOE_3D_GIZMO_NONE)
		return;
	gizmo->hovered = under;
	gizmo->hovered_view = view;

	// A refused grab captures nothing, and the press goes on to picking.
	if (!pressed)
		return;
	bool grabbed =
		kind == VOE_3D_GIZMO_RINGS ?
			voe_3d_gizmo_rings_angle(at, under, ray,
						 &gizmo->grab_angle) :
		kind == VOE_3D_GIZMO_SCALES ?
			voe_3d_gizmo_scales_grab(at, placed.rotation, under,
						 ray, &grab) :
			voe_3d_gizmo_grab(at, under, ray, &grab);
	if (!grabbed)
		return;
	gizmo->held = under;
	gizmo->captured = view;
	gizmo->kind = kind;
	gizmo->grab = grab;
	gizmo->start = placed.position;
	gizmo->kept = placed.rotation;
	gizmo->scale_start = placed.scale;
}

bool voe_editor_gizmo_taking(const voe_editor_gizmo *gizmo)
{
	VOE_BASE_ASSERT(gizmo != NULL, "asking no gizmo whether it takes a press");

	return gizmo->held != VOE_3D_GIZMO_NONE;
}

voe_3d_gizmo_handle voe_editor_gizmo_marked(const voe_editor_gizmo *gizmo,
					    uint32_t view)
{
	VOE_BASE_ASSERT(gizmo != NULL, "asking no gizmo what it marks");

	if (gizmo->held != VOE_3D_GIZMO_NONE)
		return view == gizmo->captured ? gizmo->held :
						 VOE_3D_GIZMO_NONE;
	return view == gizmo->hovered_view ? gizmo->hovered :
					     VOE_3D_GIZMO_NONE;
}
