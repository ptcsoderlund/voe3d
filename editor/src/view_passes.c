// The per-view passes view_passes.h describes: the dock tree asked which views
// show, the sun's shadow passes fitted to each one's camera, then a pass begun
// on each one's target with that camera and the shadows, the world and its
// models drawn with the selection's outline, collider, every light blocker's
// box lines, faint but the selected one's (0347 point 5, 0365 point 5), every
// fog volume's the same way (0493 point 7), the fog from the view's eye, and
// gizmo (none for a prefab's part) and the scene camera's, every directional
// light's, every point light's (0360) and every meshless place's (0365)
// markers, the selected one's in the outline's colour, the brush's rings on
// the hovered ground with the gizmo hidden while brushing a landscape (0379
// point 3), the landscapes' foliage as instanced groups gathered after the
// lights and before the shadows (0442 point 5, 0464 point 4), and the pass
// ended, its counts recorded in `views` (0465); and
// the preview's shadow passes and pass, drawn with the world's camera while the
// selected entity has one and marking nothing. Every pass is lit by every
// directional light, the first as `light` and the rest from the frame (0357
// point 1); the preview light is never joined by another, as a world with no
// light row has none. Every pass is lit by the world's
// point lights (0320 point 6) and kept out of its light blockers (0347), all
// arrays in `arena` from before the shadow call, whose lamps' pass reads the
// slots (0325 point 6) and whose bounce reads the blockers, until the pass is
// open; a false from either call leaves that pass without them, which is no
// failed frame. Each shadow call is handed
// its own target, the view's or the preview's, so its bounce is that target's
// probe volume, begun, captured and relit by that call, and no view reads a
// volume another view moved (0326 point 8); each target's frame carries its
// own remembered casters too (0394). That volume stands on the view's
// eye, so turning never moves it, and its relight shadows the sun by a sun
// map of its own, never the view's cascades (0329 points 1 and 2). Every
// shadow call is handed the platform clock and timed, with its parts, into
// the hitch watch, and every foliage gather into its `gather` (frame_hitches.h,
// 0461).
// The first refused pass stops the frame there.
#include "view_passes.h"

#include <3d/fog_volume.h>

#include <base/assert.h>

#include <platform/clock.h>

#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

// The camera marker's colour (0223), and the rule every sun and point light
// marker is given as its two colours (0320 point 7, 0360 point 1): the
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

// The brush's two rings where the pointer meets the ground (0379 point 3),
// zeroed when there is no hover hit: coloured from the gizmo's rest colour at
// the weakest strength to the outline's at the strongest.
static voe_3d_brush_marked brush_marked(const voe_editor_sculpt *sculpt,
					const voe_ui_theme *palette,
					voe_3d_material material, float pixels,
					voe_platform_size size)
{
	VOE_BASE_ASSERT(sculpt != NULL && palette != NULL,
			"marking the brush with no sculpt or palette");
	if (!sculpt->hit)
		return (voe_3d_brush_marked){ 0 };
	float t = (sculpt->strength - VOE_EDITOR_SCULPT_STRENGTH_MIN) /
		  (VOE_EDITOR_SCULPT_STRENGTH_MAX -
		   VOE_EDITOR_SCULPT_STRENGTH_MIN);
	voe_3d_brush_marked brush = {
		.entity = sculpt->entity,
		.x = sculpt->x,
		.z = sculpt->z,
		.radius = sculpt->radius,
		.inner = sculpt->radius * (1.0f - sculpt->softness),
		.material = material,
		.colour = voe_math_float3_lerp(
			voe_editor_view_gizmo_colour(palette, false),
			voe_editor_view_outline_colour(palette), t),
		.pixels = pixels,
		.size = size
	};
	VOE_BASE_ASSERT(brush.inner <= brush.radius,
			"a brush's full-weight radius past its radius");
	return brush;
}

// The foliage gather, its seconds added to the watch's `gather`; false draws
// no foliage, which is no failed frame.
static void foliage_watched(voe_ecs_world *world, voe_render_device *gpu,
			    voe_3d_frame *frame, voe_base_arena *arena,
			    voe_editor_frame_hitches *watch)
{
	double before = voe_platform_clock_now();

	VOE_BASE_ASSERT(frame != NULL && watch != NULL,
			"gathering foliage with no frame or watch");
	(void)voe_3d_draw_system_foliage(world, gpu, frame, arena);
	watch->gather += voe_platform_clock_now() - before;
	VOE_BASE_ASSERT(watch->gather >= 0.0, "a gather that took negative time");
}

// One drawn picture's counts appended to `views`' list for the Frame panel
// (0465 point 1); there is room for every view and the preview.
static void picture_add(voe_editor_views *views, voe_3d_view_counts counts,
			uint32_t view, bool preview)
{
	VOE_BASE_ASSERT(views != NULL, "recording a picture with no views");
	VOE_BASE_ASSERT(views->picture_count < VOE_EDITOR_VIEWS + 1,
			"more pictures than views and the preview");
	views->pictures[views->picture_count++] = (voe_editor_picture_counts){
		.counts = counts, .view = view, .preview = preview
	};
}

// The shadows call, its seconds added to the watch's `shadows` and the parts
// it timed by `frame->clock` to its `parts`.
static bool shadows_watched(voe_ecs_world *world, voe_render_device *gpu,
			    voe_3d_frame *frame, voe_editor_frame_hitches *watch)
{
	double before = voe_platform_clock_now();
	bool passed;

	VOE_BASE_ASSERT(frame != NULL && watch != NULL,
			"casting shadows with no frame or watch");
	passed = voe_3d_draw_system_shadows(world, gpu, frame);
	watch->shadows += voe_platform_clock_now() - before;
	watch->parts.casters += frame->shadow_times.casters;
	watch->parts.ground += frame->shadow_times.ground;
	watch->parts.foliage += frame->shadow_times.foliage;
	watch->parts.lamps += frame->shadow_times.lamps;
	watch->parts.bounce += frame->shadow_times.bounce;
	watch->parts.bounce_fit += frame->shadow_times.bounce_fit;
	watch->parts.bounce_stale += frame->shadow_times.bounce_stale;
	watch->parts.bounce_begin += frame->shadow_times.bounce_begin;
	watch->parts.bounce_captures += frame->shadow_times.bounce_captures;
	watch->parts.bounce_suns += frame->shadow_times.bounce_suns;
	watch->parts.bounce_relight += frame->shadow_times.bounce_relight;
	watch->parts.bounced += frame->shadow_times.bounced;
	VOE_BASE_ASSERT(watch->shadows >= 0.0, "shadows that took negative time");
	return passed;
}

bool voe_editor_view_passes_preview(voe_render_device *gpu,
				    voe_base_arena *arena,
				    voe_ecs_world *world,
				    voe_editor_views *views,
				    voe_render_light light,
				    const voe_editor_scene *scene,
				    const voe_3d_models *models,
				    voe_editor_frame_hitches *watch)
{
	voe_3d_frame frame;
	voe_render_pass_camera camera;
	struct voe_base_arena_mark foliage;
	struct voe_base_arena_mark mark;
	bool passed;

	VOE_BASE_ASSERT(gpu != NULL && arena != NULL && world != NULL,
			"drawing the preview with no device, arena or world");
	VOE_BASE_ASSERT(views != NULL && scene != NULL && watch != NULL,
			"drawing the preview with no views, scene or watch");

	views->preview_shown = false;
	views->picture_count = 0;
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
	frame.casters = &views->preview_casters;
	frame.clock = voe_platform_clock_now;

	foliage = voe_base_arena_mark(arena);
	// False leaves the preview unblocked or unlit by points, which is no
	// failed frame. Before the shadows: their point-shadow pass reads the
	// slots (0325) and their bounce the blockers (0347 point 4).
	(void)voe_3d_draw_system_light_blockers(world, &frame, arena);
	(void)voe_3d_draw_system_lights(world, &frame, arena);
	(void)voe_3d_draw_system_point_lights(world, &frame, arena);
	// After the lights, so it knows every sun whose cascades sort its
	// copies (0464 point 4); before the shadows, whose cascades draw the
	// same groups (0442 point 5); and before the mark, so the groups and
	// the lights outlive its rewind until the pass has drawn them.
	foliage_watched(world, gpu, &frame, arena, watch);
	mark = voe_base_arena_mark(arena);
	// Timed apart for the hitch watch (frame_hitches.h): the bounce is in it.
	passed = shadows_watched(world, gpu, &frame, watch);
	if (!passed) {
		voe_base_arena_rewind(arena, foliage);
		return false;
	}
	camera = voe_3d_draw_system_camera(&frame);
	passed = voe_render_pass_begin(gpu, views->preview_target, &camera);
	voe_base_arena_rewind(arena, mark);
	if (!passed) {
		voe_base_arena_rewind(arena, foliage);
		return false;
	}
	picture_add(views, voe_3d_draw_system_run(world, gpu, arena, frame), 0,
		    true);
	voe_render_pass_end(gpu);
	voe_base_arena_rewind(arena, foliage);
	views->preview_shown = true;
	return true;
}

bool voe_editor_view_passes_draw(
	voe_render_device *gpu, voe_base_arena *arena, voe_ecs_world *world,
	voe_editor_views *views, const voe_editor_dock_root *root,
	voe_render_light light, const voe_editor_scene *scene,
	const voe_3d_shape_geometries *geometries, const voe_3d_models *models,
	const voe_3d_shapes *shapes, const voe_ui_theme *palette, const voe_editor_gizmo *gizmo,
	float pixels_per_millimetre, voe_editor_frame_hitches *watch)
{
	VOE_BASE_ASSERT(gpu != NULL && arena != NULL && world != NULL &&
				watch != NULL,
			"drawing views with no device, arena, world or watch");
	VOE_BASE_ASSERT(views != NULL && root != NULL && scene != NULL,
			"drawing views with no views, root or scene");
	VOE_BASE_ASSERT(geometries != NULL && shapes != NULL &&
				palette != NULL && gizmo != NULL,
			"drawing views with no shapes, palette or gizmo");

	// The world's camera, zeroed for none; outline-coloured while selected.
	voe_ecs_entity camera_entity = { 0 };
	if (voe_scene_camera_count(world) > 0)
		camera_entity = voe_scene_camera_entities(world)[0];
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	voe_math_float3 camera_colour =
		marker_colour(palette, camera_entity, selected);
	// A prefab's part keeps its outline but has no gizmo (0283 point 5),
	// and a chosen brush on a landscape hides it (0379 point 3).
	bool brushing = scene->sculpt.chosen &&
			voe_editor_sculpt_wears(world, selected);
	voe_ecs_entity gizmo_entity =
		voe_editor_inspector_is_part(world, selected, NULL) ?
			(voe_ecs_entity){ 0 } :
			selected;

	// A device made with a pass per view and one more does not refuse
	// these; if it did, the caller still closes the frame and stops.
	for (uint32_t v = 0; v < views->count; v++) {
		voe_editor_view *view = &views->views[v];
		voe_render_pass_camera camera;
		voe_3d_frame frame;

		if (!voe_editor_dock_shows_view(root, v))
			continue;
		voe_platform_size size = { (int)view->width, (int)view->height };

		// This view's own view and eye first: its cascades are fitted
		// to them before its pass opens.
		camera = voe_editor_view_pass_camera(view, light);
		frame = (voe_3d_frame){ .view = camera.view,
					.light = camera.light,
					.sky = voe_3d_draw_system_sky(world, view->eye),
					.clouds = voe_3d_draw_system_clouds(world,
									    view->eye),
					// At lag 0, as this frame sets none (0254).
					.fog = voe_3d_fog_volumes(world, view->eye,
								  0.0f),
					.exposure = voe_3d_draw_system_picture(world),
					.eye = view->eye,
					.models = models,
					.target = view->target,
					.casters = &view->casters,
					.clock = voe_platform_clock_now };
		// The points come before the shadows, whose point-shadow pass
		// reads their slots (0325), and so do the blockers, whose bounce
		// reads them (0347 point 4), and the lights after the first, which
		// read the blockers' masks; a false leaves this pass unblocked or
		// unlit by points, no failed frame.
		struct voe_base_arena_mark foliage = voe_base_arena_mark(arena);
		(void)voe_3d_draw_system_light_blockers(world, &frame, arena);
		(void)voe_3d_draw_system_lights(world, &frame, arena);
		(void)voe_3d_draw_system_point_lights(world, &frame, arena);
		// After the lights, so it knows every sun whose cascades sort its
		// copies (0464 point 4); before the shadows, whose cascades draw
		// the same groups (0442 point 5); and before the mark, so the
		// groups and the lights outlive its rewind until _run.
		foliage_watched(world, gpu, &frame, arena, watch);
		struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
		// Timed apart, with its parts, for the hitch watch (frame_hitches.h).
		bool shadowed = shadows_watched(world, gpu, &frame, watch);
		if (!shadowed) {
			voe_base_arena_rewind(arena, foliage);
			return false;
		}
		camera = voe_3d_draw_system_camera(&frame);
		bool passed = voe_render_pass_begin(gpu, view->target, &camera);
		voe_base_arena_rewind(arena, mark);
		if (!passed) {
			voe_base_arena_rewind(arena, foliage);
			return false;
		}
		voe_3d_frame drawn = (voe_3d_frame){
				.view = camera.view,
				.light = camera.light,
				.sky = frame.sky,
				.clouds = frame.clouds,
				.fog = frame.fog,
				.exposure = frame.exposure,
				.eye = view->eye,
				.shadow = frame.shadow,
				.models = models,
				.foliage = frame.foliage,
				.foliage_count = frame.foliage_count,
				.counts = frame.counts,
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
					.kind = scene->gizmo_kind,
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
				// Every place with no mesh (0365 points 1-2).
				.places = {
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
						  (int)view->height } },
				// Every blocker faint, the selected one bright (0365 point 5).
				.light_blockers = {
					.shown = true,
					.selected = selected,
					.material = shapes->outline,
					.colour = voe_editor_view_faint_colour(
						palette),
					.selected_colour =
						voe_editor_view_outline_colour(
							palette),
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				// Every fog volume's box as the blockers' (0493 point 7).
				.fog_volumes = {
					.shown = true,
					.selected = selected,
					.material = shapes->outline,
					.colour = voe_editor_view_faint_colour(
						palette),
					.selected_colour =
						voe_editor_view_outline_colour(
							palette),
					.pixels = VOE_EDITOR_OUTLINE_MILLIMETRES *
						  pixels_per_millimetre,
					.size = { (int)view->width,
						  (int)view->height } },
				// The brush where the pointer meets the ground.
				.brush = brush_marked(
					&scene->sculpt, palette,
					shapes->outline,
					VOE_EDITOR_OUTLINE_MILLIMETRES *
						pixels_per_millimetre,
					size) };
		if (brushing)
			drawn.gizmo = (voe_3d_gizmoed){ 0 };
		picture_add(views, voe_3d_draw_system_run(world, gpu, arena, drawn),
			    v, false);
		voe_render_pass_end(gpu);
		voe_base_arena_rewind(arena, foliage);
	}
	VOE_BASE_ASSERT(views->count <= VOE_EDITOR_VIEWS,
			"more views than there is room for");
	return true;
}
