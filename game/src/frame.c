// The systems, the light blockers, the point lights, the lights after the first
// and the foliage's groups in `scratch`, rewound once the window pass has drawn
// them, the shadow
// passes (each casting light's cascades, the frame's target left zero, the
// window's, and the lamps'), then the bounce's capture passes, each bouncing
// sun's map when it relights, then the relight when a light bounces, and the
// window pass, its camera voe_3d_draw_system_camera's, its counts handed back,
// with the interface over
// the world, in the order game/include/game/frame.h gives. A refused pass, shadow or window, still
// closes the draw, so the frame ends as render expects and the caller is told
// once. The world step drains in order, the haze's after the sky's, the
// sounds last with no mixer.
#include <game/frame.h>

#include <audio/sound_system.h>

#include <3d/draw_system.h>
#include <3d/model_component.h>

#include <base/assert.h>

#include <ecs/structure.h>

#include <game/project.h>

#include <physics/body_system.h>
#include <physics/collider_system.h>

#include <platform/clock.h>

#include <scene/haze_system.h>
#include <scene/identity_system.h>
#include <scene/light_blocker_system.h>
#include <scene/light_system.h>
#include <scene/picture_system.h>
#include <scene/sky_system.h>
#include <scene/transform_system.h>

#include <ui/widgets.h>

void voe_game_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes)
{
	VOE_BASE_ASSERT(world != NULL && shapes != NULL,
			"a world step with no world or shapes");

	// Which rows exist changes here and nowhere else in the frame
	// (ecs/structure.h), before any system reads a table.
	voe_ecs_structure_apply(world);
	voe_game_project_replaces_apply(world);
	voe_scene_transform_system_run(world);
	voe_scene_identity_system_run(world);
	voe_scene_light_system_run(world);
	voe_scene_sky_system_run(world);
	voe_scene_haze_system_run(world);
	voe_scene_picture_system_run(world);
	voe_scene_light_blocker_system_run(world);
	voe_3d_shape_system_run(world, shapes);
	voe_3d_model_system_run(world);
	voe_physics_collider_system_run(world);
	voe_physics_body_system_run(world);
	// No mixer: replaces and controls applied, nothing played (0304 point 7).
	voe_audio_sound_system_run(world, NULL, 1.0f);
}

// The interface's records submitted as one range and drawn in one command
// over the world, depth cleared first so no surface hides them. False when a
// record did not fit or the draw was refused.
static bool draw_interface(voe_render_device *device, const voe_ui_context *ui,
			   voe_platform_size size)
{
	uint32_t records = ui == NULL ? 0 : voe_ui_element_count(ui);
	uint32_t first;
	bool ok = true;

	if (records == 0)
		return true;
	voe_render_frame_clear_depth(device);
	first = voe_render_frame_elements_submitted(device);
	for (uint32_t e = 0; e < records && ok; e++)
		ok = voe_render_frame_submit_element(device,
						     voe_ui_element(ui, e));
	return ok && voe_render_frame_draw_elements(
			     device,
			     voe_render_element_transform(
				     voe_game_interface_surface(size)),
			     first, records);
}

bool voe_game_frame(voe_app *app, voe_ecs_world *world,
		    const voe_3d_shapes *shapes, const voe_3d_models *models,
		    voe_base_arena *scratch,
		    voe_platform_size size, float lag,
		    voe_3d_bounce_casters *casters, voe_3d_lods_kept *lods,
		    const voe_ui_context *ui, voe_3d_view_counts *counts)
{
	voe_render_device *device;
	voe_render_pass_camera camera;
	voe_3d_frame frame;
	struct voe_base_arena_mark foliage;
	struct voe_base_arena_mark mark;
	bool drawing;
	bool shadowed;
	bool passed;

	VOE_BASE_ASSERT(app != NULL && world != NULL && shapes != NULL &&
				scratch != NULL,
			"a game frame with no app, world, shapes or scratch");
	VOE_BASE_ASSERT(lag >= 0.0f && lag <= 1.0f,
			"a game frame lag outside 0 to 1");

	voe_game_world_step(world, shapes);

	if (!voe_app_draw_open(app, size, &drawing))
		return false;
	if (!drawing)
		return true;

	device = voe_app_device(app);
	frame = voe_3d_draw_system_frame(world, size, lag);
	frame.models = models;
	frame.casters = casters;
	// Wall time, once a frame drawn and before the shadows choose: a
	// paused Play still ends a dissolve (0501 point 2).
	if (lods != NULL)
		voe_3d_lods_kept_begin(lods, voe_platform_clock_now());
	frame.lods = lods;
	// The lights and the foliage's groups stay on `scratch` until the
	// window pass has drawn them.
	foliage = voe_base_arena_mark(scratch);
	// False leaves the pass unblocked, which is no failed frame. Before
	// the shadows: the bounce is handed the same blockers (0347 point 4).
	(void)voe_3d_draw_system_light_blockers(world, &frame, scratch);
	// False leaves the frame unlit by points, which is no failed frame.
	// Before the shadows: their point-shadow pass reads the slots (0325).
	(void)voe_3d_draw_system_point_lights(world, &frame, scratch);
	// False leaves the frame lit by the first light alone, which is no
	// failed frame. After the blockers, whose masks it reads, and before
	// the shadows, which set its lights' shadows (0357 point 1).
	(void)voe_3d_draw_system_lights(world, &frame, scratch);
	// False draws no foliage, which is no failed frame. After the lights,
	// so it knows every sun (0464 point 4), and before the shadows: each
	// of their cascades draws the same groups (0442 point 5).
	(void)voe_3d_draw_system_foliage(world, device, &frame, scratch);
	mark = voe_base_arena_mark(scratch);
	shadowed = voe_3d_draw_system_shadows(world, device, &frame);
	camera = voe_3d_draw_system_camera(&frame);
	passed = voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
				       &camera);
	voe_base_arena_rewind(scratch, mark);
	if (passed) {
		const voe_3d_view_counts drawn =
			voe_3d_draw_system_run(world, device, scratch, frame);

		if (counts != NULL)
			*counts = drawn;
		passed = draw_interface(device, ui, size);
		voe_render_pass_end(device);
	}
	voe_base_arena_rewind(scratch, foliage);
	return voe_app_draw_close(app) && shadowed && passed;
}
