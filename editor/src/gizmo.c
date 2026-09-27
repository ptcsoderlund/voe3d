// The hover, the press that grabs a handle or a ring and the move or turn that follows it, each
// against a gizmo built from the view's own camera and picture size. See the
// header for why the middle button is not read here, why the drag is measured
// from the press, and why a whole transform is submitted.
#include "gizmo.h"

#include <3d/gizmo_rings.h>
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

// `row`'s rotation replaced by the kept one turned about the held ring's world
// axis by the angle swept since the press (0274), pre-multiplied so the turn is
// the world's and not the entity's own, and normalized so a long drag hands the
// drain a unit rotation. False, `row` untouched, for a refused angle (the last
// submitted stands) or a rotation that has not changed.
static bool gizmo_turn(const voe_editor_gizmo *gizmo, voe_3d_gizmo at,
		       voe_3d_ray ray, voe_scene_transform *row)
{
	float angle;

	if (!voe_3d_gizmo_rings_angle(at, gizmo->held, ray, &angle))
		return false;

	voe_math_quat rotation = voe_math_quat_normalize(voe_math_quat_mul(
		voe_math_quat_from_axis_angle(gizmo_ring_axis(gizmo->held),
					      angle - gizmo->grab_angle),
		gizmo->kept));
	if (rotation.x == row->rotation.x && rotation.y == row->rotation.y &&
	    rotation.z == row->rotation.z && rotation.w == row->rotation.w)
		return false;
	row->rotation = rotation;
	return true;
}

// One frame of a running drag in the captured view, wherever the pointer is
// now. The point is worked out as voe_editor_views_under does, but for this view
// even once the pointer has left it, as the views' own drag goes on.
static void gizmo_drag(voe_editor_gizmo *gizmo, voe_ecs_world *world,
		       voe_ecs_entity entity, voe_scene_transform row,
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

	if (gizmo->turning) {
		if (!gizmo_turn(gizmo, at, ray, &row))
			return;
	} else {
		// A refused grab is a glancing ray this frame (3d/gizmo.h): the
		// entity keeps the position it had.
		if (!voe_3d_gizmo_grab(at, gizmo->held, ray, &now))
			return;

		voe_math_double3 position = voe_math_double3_add(
			gizmo->start, voe_math_double3_sub(now, gizmo->grab));
		if (position.x == row.position.x &&
		    position.y == row.position.y &&
		    position.z == row.position.z)
			return;
		row.position = position;
	}

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

	const voe_scene_transform *row =
		scene->world == NULL ?
			NULL :
			voe_scene_transform_get(scene->world, scene->selected);
	if (blocked || row == NULL || !down || gizmo->turning != scene->rings)
		gizmo->held = VOE_3D_GIZMO_NONE;
	if (blocked || row == NULL)
		return;

	if (gizmo->held != VOE_3D_GIZMO_NONE) {
		VOE_BASE_ASSERT(gizmo->captured < views->count,
				"a drag captured in no view");
		gizmo_drag(gizmo, scene->world, scene->selected, *row,
			   &views->views[gizmo->captured], pixels, pointer);
		return;
	}

	if (!voe_editor_views_under(views, pointer, &view, &point))
		return;

	voe_3d_ray ray = gizmo_in_view(&views->views[view], point, pixels,
				       row->position, &at);
	voe_3d_gizmo_handle under = scene->rings ?
					    voe_3d_gizmo_rings_hit(at, ray) :
					    voe_3d_gizmo_hit(at, ray);
	if (under == VOE_3D_GIZMO_NONE)
		return;
	gizmo->hovered = under;
	gizmo->hovered_view = view;

	// A refused grab captures nothing, and the press goes on to picking.
	if (!pressed)
		return;
	if (scene->rings ?
		    !voe_3d_gizmo_rings_angle(at, under, ray, &gizmo->grab_angle) :
		    !voe_3d_gizmo_grab(at, under, ray, &grab))
		return;
	gizmo->held = under;
	gizmo->captured = view;
	gizmo->turning = scene->rings;
	gizmo->grab = grab;
	gizmo->start = row->position;
	gizmo->kept = row->rotation;
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
