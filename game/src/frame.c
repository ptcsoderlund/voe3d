// The systems, the point lights in `scratch`, rewound once the window pass
// has copied them, the shadow passes (the sun's, which also feed the bounce,
// the frame's target left zero, the window's, and the lamps'), and the window pass with the interface over
// the world, in the order game/include/game/frame.h gives. A refused pass, shadow or window, still
// closes the draw, so the frame ends as render expects and the caller is told
// once.
#include <game/frame.h>

#include <audio/sound_system.h>

#include <3d/draw_system.h>
#include <3d/model_component.h>

#include <base/assert.h>

#include <ecs/structure.h>

#include <game/project.h>

#include <physics/body_system.h>
#include <physics/collider_system.h>

#include <scene/identity_system.h>
#include <scene/light_system.h>
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
		    voe_platform_size size, float lag, const voe_ui_context *ui)
{
	voe_render_device *device;
	voe_render_pass_camera camera;
	voe_3d_frame frame;
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
	mark = voe_base_arena_mark(scratch);
	// False leaves the frame unlit by points, which is no failed frame.
	// Before the shadows: their point-shadow pass reads the slots (0325).
	(void)voe_3d_draw_system_point_lights(world, &frame, scratch);
	shadowed = voe_3d_draw_system_shadows(world, device, &frame);
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow,
					   .points = frame.points };
	passed = voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
				       &camera);
	voe_base_arena_rewind(scratch, mark);
	if (passed) {
		voe_3d_draw_system_run(world, device, scratch, frame);
		passed = draw_interface(device, ui, size);
		voe_render_pass_end(device);
	}
	return voe_app_draw_close(app) && shadowed && passed;
}
