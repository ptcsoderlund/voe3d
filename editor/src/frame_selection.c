// The selection's size asked of `3d`, or its world position when it has none,
// turned into a focus and distance for the view under the pointer to glide to.
// See frame_selection.h for why only that view and why a fixed distance.
#include "frame_selection.h"

#include <3d/bounds.h>

#include <base/assert.h>

#include <scene/transform_component.h>

void voe_editor_frame_selection(voe_editor_views *views,
				const voe_editor_scene *scene,
				const voe_3d_shape_geometries *geometries,
				const voe_3d_models *models,
				voe_math_float2 pointer)
{
	uint32_t index;
	voe_math_float2 point;
	voe_math_double3 focus;
	float radius;
	float distance;

	VOE_BASE_ASSERT(views != NULL && scene != NULL,
			"framing with no views or scene");
	VOE_BASE_ASSERT(geometries != NULL, "framing against no geometries");

	if (scene->world == NULL ||
	    !voe_ecs_entity_alive(scene->world, scene->selected))
		return;
	if (!voe_editor_views_under(views, pointer, &index, &point))
		return;

	voe_editor_view *view = &views->views[index];
	if (voe_3d_bounds(scene->world, geometries, models, scene->selected,
			  &focus, &radius)) {
		// The picture's own size is the aspect it is drawn at (view.h),
		// and the size spans two thirds of its narrower side.
		const float aspect =
			view->height > 0 ?
				(float)view->width / (float)view->height :
				1.0f;
		distance = voe_3d_bounds_distance(radius, view->lens.fov_y,
						  aspect, 2.0f / 3.0f);
	} else if (voe_scene_transform_get(scene->world, scene->selected) !=
		   NULL) {
		focus = voe_scene_transform_world(scene->world, scene->selected)
				.position;
		distance = VOE_EDITOR_FRAME_NEAR;
	} else {
		return;
	}

	voe_editor_view_glide_to(view, focus, distance);
	VOE_BASE_ASSERT(view->gliding, "a frame that did not start a glide");
}
