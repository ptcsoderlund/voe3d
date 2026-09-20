// The press edge and the view it landed in, handed to `3d` as a ray. See the
// header for why the middle button is not read here, what `blocked` is for, why
// a press and not a release selects, and why a click over no view leaves the
// selection alone.
#include "pick.h"

#include <3d/pick.h>

#include <base/assert.h>

void voe_editor_pick_read(voe_editor_pick *pick, voe_editor_scene *scene,
			  const voe_editor_views *views,
			  const voe_3d_shape_geometries *geometries,
			  voe_math_float2 pointer, bool down, bool blocked)
{
	bool pressed;
	uint32_t view;
	voe_math_float2 point;

	VOE_BASE_ASSERT(pick != NULL, "reading a press into no pick");
	VOE_BASE_ASSERT(scene != NULL, "picking into no scene");
	VOE_BASE_ASSERT(views != NULL, "picking in no views");
	VOE_BASE_ASSERT(geometries != NULL, "picking against no geometries");

	pressed = down && !pick->pointer_was_down;
	pick->pointer_was_down = down;
	if (blocked || !pressed)
		return;

	// Over no view is not an answer: the press was somewhere else in the
	// editor and says nothing about what is selected.
	if (!voe_editor_views_under(views, pointer, &view, &point))
		return;

	const voe_editor_view *clicked = &views->views[view];
	voe_3d_ray ray = voe_3d_pick_ray(
		clicked->camera,
		(voe_platform_size){ (int)clicked->width,
				     (int)clicked->height },
		point);

	// A zeroed entity is what the ray meeting nothing answers, and
	// selecting one clears the selection (scene.h) — which is what a click
	// on empty space in a view means.
	voe_editor_scene_select(
		scene, voe_3d_pick(scene->world, geometries, ray, NULL));
}
