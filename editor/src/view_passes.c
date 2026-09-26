// The per-view passes view_passes.h describes: the dock tree asked which views
// show, the sun's shadow passes fitted to each one's camera, then a pass begun
// on each one's target with that camera and the shadows, the world drawn
// with the selection's outline, collider and gizmo and the scene camera's
// marker, and
// the pass ended; and the preview's shadow passes and pass, drawn with the
// world's camera while the selected entity has one.
#include "view_passes.h"

#include <base/assert.h>

#include <scene/camera_component.h>

bool voe_editor_view_passes_preview(voe_render_device *gpu,
				    voe_base_arena *arena,
				    voe_ecs_world *world,
				    voe_editor_views *views,
				    voe_render_light light,
				    const voe_editor_scene *scene)
{
	voe_3d_frame frame;
	voe_render_pass_camera camera;

	VOE_BASE_ASSERT(gpu != NULL && arena != NULL && world != NULL,
			"drawing the preview with no device, arena or world");
	VOE_BASE_ASSERT(views != NULL && scene != NULL,
			"drawing the preview with no views or scene");

	views->preview_shown = false;
	if (voe_scene_camera_get(world, voe_editor_scene_selected(scene)) ==
	    NULL)
		return true;

	// Outline, gizmo and marker come back zeroed and stay so (0223); the
	// eye is the world camera's, and the editor draws what is (0254).
	frame = voe_3d_draw_system_frame(
		world,
		(voe_platform_size){ VOE_EDITOR_PREVIEW_WIDTH,
				     VOE_EDITOR_PREVIEW_HEIGHT },
		0.0f);
	if (frame.blind)
		return true;
	frame.light = light;
	if (!voe_3d_draw_system_shadows(world, gpu, &frame))
		return false;

	camera = (voe_render_pass_camera){ frame.view, frame.light,
					   frame.shadow };
	if (!voe_render_pass_begin(gpu, views->preview_target, &camera))
		return false;
	voe_3d_draw_system_run(world, gpu, arena, frame);
	voe_render_pass_end(gpu);
	views->preview_shown = true;
	return true;
}

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

	// The world's camera, zeroed for none; outline-coloured while selected.
	voe_ecs_entity camera_entity = { 0 };
	if (voe_scene_camera_count(world) > 0)
		camera_entity = voe_scene_camera_entities(world)[0];
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	bool camera_selected = camera_entity.generation != 0 &&
			       camera_entity.index == selected.index &&
			       camera_entity.generation == selected.generation;
	voe_math_float3 marker_colour =
		camera_selected ? voe_editor_view_outline_colour(palette)
				: voe_editor_view_gizmo_colour(palette, false);

	// A device made with a pass per view and one more does not refuse
	// these; if it did, the caller still closes the frame and stops.
	for (uint32_t v = 0; v < views->count; v++) {
		const voe_editor_view *view = &views->views[v];
		voe_render_pass_camera camera;
		voe_3d_frame frame;

		if (!voe_editor_dock_shows_view(tree, v))
			continue;

		// This view's own view and eye first: its cascades are fitted
		// to them before its pass opens.
		camera = voe_editor_view_pass_camera(view, light);
		frame = (voe_3d_frame){ .view = camera.view,
					.light = camera.light,
					.eye = view->eye };
		if (!voe_3d_draw_system_shadows(world, gpu, &frame))
			return false;
		camera.shadow = frame.shadow;
		if (!voe_render_pass_begin(gpu, view->target, &camera))
			return false;
		voe_3d_draw_system_run(
			world, gpu, arena,
			(voe_3d_frame){
				.view = camera.view,
				.light = camera.light,
				.eye = view->eye,
				.shadow = frame.shadow,
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
						  (int)view->height } },
				.marker = {
					.entity = camera_entity,
					.material = shapes->outline,
					.colour = marker_colour,
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				// The selection; one without a collider draws none.
				.collider = {
					.entity = selected,
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } } });
		voe_render_pass_end(gpu);
	}
	VOE_BASE_ASSERT(views->count <= VOE_EDITOR_VIEWS,
			"more views than there is room for");
	return true;
}
