// The drag's start from the held row and its release into a view, onto the
// Inspector or nowhere. See the header for the four outcomes, the point a
// placed model lands at and why the Inspector drop replaces the path.
#include "assets_drag.h"

#include "entities.h"

#include <3d/pick.h>

#include <base/assert.h>

#include <stdio.h>

// How far along the ray a model lands that meets nothing and no ground.
#define DROP_METRES 10.0

// The held row's path into `drag`, `\` from a joined subfolder made `/`, and
// whether it is a prefab. False when it does not fit its kind's row.
static bool path_hold(voe_editor_assets_drag *drag,
		      const voe_editor_assets *assets)
{
	size_t room = assets->held_prefab ? VOE_SCENE_PREFAB_PATH :
					    VOE_3D_MODEL_PATH;
	int length = assets->shown[0] == '\0' ?
			     snprintf(drag->path, sizeof drag->path,
				      "Assets/%s", assets->held) :
			     snprintf(drag->path, sizeof drag->path,
				      "Assets/%s/%s", assets->shown,
				      assets->held);

	VOE_BASE_ASSERT(room <= sizeof drag->path, "a row longer than the drag");
	if (length < 0 || (size_t)length >= room)
		return false;
	drag->prefab = assets->held_prefab;
	for (int i = 0; i < length; i++)
		if (drag->path[i] == '\\')
			drag->path[i] = '/';
	return true;
}

// The hit, else y = 0 in front of the eye, else DROP_METRES along the ray.
static voe_math_double3 drop_point(voe_ecs_world *world,
				   const voe_3d_shape_geometries *geometries,
				   const voe_3d_models *models, voe_3d_ray ray)
{
	float distance = 0.0f;
	double along = DROP_METRES;

	if (voe_3d_pick(world, geometries, models, ray, &distance).generation !=
	    0)
		along = distance;
	else if (ray.direction.y != 0.0f && -ray.origin.y / ray.direction.y > 0.0)
		along = -ray.origin.y / ray.direction.y;
	return (voe_math_double3){ ray.origin.x + ray.direction.x * along,
				   ray.origin.y + ray.direction.y * along,
				   ray.origin.z + ray.direction.z * along };
}

// A new thing wearing the held path, or a placed copy of the held prefab,
// where the view's ray lands, selected.
static bool drop_into_view(const voe_editor_assets_drag *drag,
			   voe_editor_scene *scene,
			   const voe_editor_view *view, voe_math_float2 point,
			   const voe_3d_shape_geometries *geometries,
			   const voe_3d_models *models)
{
	voe_ecs_entity made;
	voe_3d_ray ray = voe_3d_pick_ray(
		voe_editor_view_pass_camera(view, (voe_render_light){ 0 }).view,
		view->eye,
		(voe_platform_size){ (int)view->width, (int)view->height },
		point);
	voe_math_double3 at = drop_point(scene->world, geometries, models, ray);

	if (drag->prefab ?
		    !voe_editor_entities_prefab_add(scene->world, drag->path, at,
						    &made) :
		    !voe_editor_entities_model_add(scene->world, drag->path, at,
						   &made))
		return false;
	voe_editor_scene_select(scene, made);
	return true;
}

// The release: into a view, onto the Inspector, or nothing. Whether an edit
// reached the project; a full queue says so in the notice.
static void drop(const voe_editor_assets_drag *drag,
		 voe_editor_session *session, voe_editor_undo *undo,
		 voe_editor_scene *scene, const voe_editor_views *views,
		 const voe_editor_dock_root *root, const voe_editor_topbar *bar,
		 const voe_3d_shape_geometries *geometries,
		 const voe_3d_models *models, voe_math_float2 pointer)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	voe_3d_model_intent swap = { .entity = selected };
	uint32_t view;
	voe_math_float2 point;
	bool done;

	if (voe_editor_views_under(views, pointer, &view, &point)) {
		done = drop_into_view(drag, scene, &views->views[view], point,
				      geometries, models);
	} else if (voe_editor_dock_over_panel(
			   root, voe_editor_topbar_high(bar, root->size.y),
			   VOE_EDITOR_PANEL_INSPECTOR, pointer) &&
		   !drag->prefab &&
		   !voe_editor_inspector_is_part(scene->world, selected, NULL) &&
		   voe_3d_model_get(scene->world, selected) != NULL) {
		snprintf(swap.model.path, sizeof swap.model.path, "%s",
			 drag->path);
		done = voe_3d_model_submit(scene->world, swap);
	} else {
		return;
	}
	if (!done) {
		voe_editor_notice_set(&session->notice, "The scene is full.");
		return;
	}
	voe_editor_session_edited(session);
	voe_editor_undo_edited(undo);
}

void voe_editor_assets_drag_read(
	voe_editor_assets_drag *drag, voe_editor_session *session,
	voe_editor_undo *undo, voe_editor_scene *scene,
	const voe_editor_views *views, const voe_editor_dock_root *root,
	const voe_editor_topbar *bar, const voe_3d_shape_geometries *geometries,
	const voe_3d_models *models, voe_math_float2 pointer, bool down,
	bool blocked)
{
	VOE_BASE_ASSERT(drag != NULL && session != NULL && undo != NULL,
			"dragging with nothing to drag into");
	VOE_BASE_ASSERT(scene != NULL && views != NULL && root != NULL &&
				bar != NULL && geometries != NULL,
			"dragging over no editor");

	if (!drag->holding) {
		drag->holding = down && !blocked &&
				scene->assets.held != NULL &&
				path_hold(drag, &scene->assets);
		return;
	}
	if (down)
		return;
	drag->holding = false;
	if (!blocked)
		drop(drag, session, undo, scene, views, root, bar, geometries,
		     models, pointer);
}
