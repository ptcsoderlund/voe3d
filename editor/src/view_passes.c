// The per-view passes view_passes.h describes: the dock tree asked which views
// show, the sun's shadow passes fitted to each one's camera, then a pass begun
// on each one's target with that camera and the shadows, the world and its
// models drawn with the selection's outline, collider and gizmo (none for a
// prefab's part) and the scene camera's, the sun's and every point light's
// markers, and the pass ended; and the preview's shadow passes and pass, drawn
// with the world's camera while the selected entity has one and marking
// nothing. Every pass is lit by the world's point lights (0320 point 6), their
// array in `arena` from before the shadow call, whose lamps' pass reads their
// slots (0325 point 6), until the pass is open. Each shadow call is handed
// its own target, the view's or the preview's, so its bounce is that target's
// probe volume, begun, captured and relit by that call, and no view reads a
// volume another view moved (0326 point 8). That volume stands on the view's
// eye, so turning never moves it, and its relight shadows the sun by a sun
// map of its own, never the view's cascades (0329 points 1 and 2).
#include "view_passes.h"

#include <base/assert.h>

#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

// A marker's colour, the camera's and the sun's alike (0223, 0274), and the rule
// the point light markers are given as their two colours (0320 point 7): the
// outline's while `marked` is the selection, the gizmo's rest colour otherwise.
static voe_math_float3 marker_colour(const voe_ui_theme *palette,
				     voe_ecs_entity marked,
				     voe_ecs_entity selected)
{
	bool is_selected = marked.generation != 0 &&
			   marked.index == selected.index &&
			   marked.generation == selected.generation;
	return is_selected ? voe_editor_view_outline_colour(palette)
			   : voe_editor_view_gizmo_colour(palette, false);
}

// The world's first light with a transform, zeroed for none: the one sun a
// view marks (0274).
static voe_ecs_entity first_placed_light(const voe_ecs_world *world)
{
	const voe_ecs_entity *lights = voe_scene_light_entities(world);
	for (uint32_t i = 0; i < voe_scene_light_count(world); i++)
		if (voe_scene_transform_get(world, lights[i]) != NULL)
			return lights[i];
	return (voe_ecs_entity){ 0 };
}

bool voe_editor_view_passes_preview(voe_render_device *gpu,
				    voe_base_arena *arena,
				    voe_ecs_world *world,
				    voe_editor_views *views,
				    voe_render_light light,
				    const voe_editor_scene *scene,
				    const voe_3d_models *models)
{
	voe_3d_frame frame;
	voe_render_pass_camera camera;
	struct voe_base_arena_mark mark;
	bool passed;

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
	frame.models = models;
	frame.target = views->preview_target;

	mark = voe_base_arena_mark(arena);
	// False leaves the preview unlit by points, which is no failed frame.
	// Before the shadows: their point-shadow pass reads the slots (0325).
	(void)voe_3d_draw_system_point_lights(world, &frame, arena);
	if (!voe_3d_draw_system_shadows(world, gpu, &frame)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow,
					   .points = frame.points };
	passed = voe_render_pass_begin(gpu, views->preview_target, &camera);
	voe_base_arena_rewind(arena, mark);
	if (!passed)
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
	const voe_3d_shape_geometries *geometries, const voe_3d_models *models,
	const voe_3d_shapes *shapes, const voe_ui_theme *palette, const voe_editor_gizmo *gizmo,
	float pixels_per_millimetre)
{
	VOE_BASE_ASSERT(gpu != NULL && arena != NULL && world != NULL,
			"drawing views with no device, arena or world");
	VOE_BASE_ASSERT(views != NULL && tree != NULL && scene != NULL,
			"drawing views with no views, tree or scene");
	VOE_BASE_ASSERT(geometries != NULL && shapes != NULL &&
				palette != NULL && gizmo != NULL,
			"drawing views with no shapes, palette or gizmo");

	// The world's camera and sun, zeroed for none; outline-coloured while
	// selected.
	voe_ecs_entity camera_entity = { 0 };
	if (voe_scene_camera_count(world) > 0)
		camera_entity = voe_scene_camera_entities(world)[0];
	voe_ecs_entity sun_entity = first_placed_light(world);
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	voe_math_float3 camera_colour =
		marker_colour(palette, camera_entity, selected);
	voe_math_float3 sun_colour =
		marker_colour(palette, sun_entity, selected);
	// A prefab's part keeps its outline but has no gizmo (0283 point 5).
	voe_ecs_entity gizmo_entity =
		voe_editor_inspector_is_part(world, selected, NULL) ?
			(voe_ecs_entity){ 0 } :
			selected;

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
					.eye = view->eye,
					.models = models,
					.target = view->target };
		// The points live until the pass has taken them, and come before
		// the shadows, whose point-shadow pass reads their slots (0325); a
		// false leaves this pass unlit by points, which is no failed frame.
		struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
		(void)voe_3d_draw_system_point_lights(world, &frame, arena);
		if (!voe_3d_draw_system_shadows(world, gpu, &frame)) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}
		camera.shadow = frame.shadow;
		camera.points = frame.points;
		bool passed = voe_render_pass_begin(gpu, view->target, &camera);
		voe_base_arena_rewind(arena, mark);
		if (!passed)
			return false;
		voe_3d_draw_system_run(
			world, gpu, arena,
			(voe_3d_frame){
				.view = camera.view,
				.light = camera.light,
				.eye = view->eye,
				.shadow = frame.shadow,
				.models = models,
				.outlined = {
					.models = models,
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
					.entity = gizmo_entity,
					.material = shapes->outline,
					.colour = voe_editor_view_gizmo_colour(
						palette, false),
					.marked_colour = voe_editor_view_gizmo_colour(
						palette, true),
					.marked = voe_editor_gizmo_marked(gizmo, v),
					.rings = scene->rings,
					.pixels = VOE_EDITOR_GIZMO_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				.marker = {
					.entity = camera_entity,
					.material = shapes->outline,
					.colour = camera_colour,
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				.sun = {
					.entity = sun_entity,
					.material = shapes->outline,
					.colour = sun_colour,
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				.point_lights = {
					.shown = true,
					.selected = selected,
					.material = shapes->outline,
					.colour = voe_editor_view_gizmo_colour(
						palette, false),
					.selected_colour =
						voe_editor_view_outline_colour(
							palette),
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
