// The per-view passes view_passes.h describes: the dock tree asked which views
// show, a pass begun on each one's target with its own camera, the world drawn
// with the selection's outline and gizmo, and the pass ended.
#include "view_passes.h"

#include <base/assert.h>

bool voe_editor_view_passes_draw(
	voe_render_device *gpu, voe_base_arena *arena, voe_ecs_world *world,
	const voe_editor_views *views, const voe_editor_dock_tree *tree,
	voe_render_light light, const voe_editor_scene *scene,
	const voe_3d_shape_geometries *geometries, const voe_3d_shapes *shapes,
	const voe_ui_theme *palette, const voe_editor_gizmo *gizmo,
	float pixels_per_millimetre)
{
	VOE_BASE_ASSERT(gpu != NULL && arena != NULL && world != NULL,
			"drawing views with no device, arena or world");
	VOE_BASE_ASSERT(views != NULL && tree != NULL && scene != NULL,
			"drawing views with no views, tree or scene");
	VOE_BASE_ASSERT(geometries != NULL && shapes != NULL &&
				palette != NULL && gizmo != NULL,
			"drawing views with no shapes, palette or gizmo");

	// A device made with a pass per view and one more does not refuse
	// these; if it did, the caller still closes the frame and stops.
	for (uint32_t v = 0; v < views->count; v++) {
		const voe_editor_view *view = &views->views[v];
		voe_render_pass_camera camera;

		if (!voe_editor_dock_shows_view(tree, v))
			continue;

		camera = voe_editor_view_pass_camera(view, light);
		if (!voe_render_pass_begin(gpu, view->target, &camera))
			return false;
		voe_3d_draw_system_run(
			world, gpu, arena,
			(voe_3d_frame){
				.view = camera.view,
				.light = camera.light,
				.outlined = {
					.entity = voe_editor_scene_selected(scene),
					.geometries = geometries,
					.material = shapes->outline,
					.colour = voe_editor_view_outline_colour(
						palette),
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				.gizmo = {
					.entity = voe_editor_scene_selected(scene),
					.material = shapes->outline,
					.colour = voe_editor_view_gizmo_colour(
						palette, false),
					.marked_colour = voe_editor_view_gizmo_colour(
						palette, true),
					.marked = voe_editor_gizmo_marked(gizmo, v),
					.pixels = VOE_EDITOR_GIZMO_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } } });
		voe_render_pass_end(gpu);
	}
	VOE_BASE_ASSERT(views->count <= VOE_EDITOR_VIEWS,
			"more views than there is room for");
	return true;
}
