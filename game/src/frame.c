// The systems and the one window pass, in the order game/include/game/frame.h
// gives. A refused pass still closes the draw, so the frame ends as render
// expects and the caller is told once.
#include <game/frame.h>

#include <3d/draw_system.h>

#include <base/assert.h>

#include <ecs/structure.h>

#include <game/project.h>

#include <physics/body_system.h>
#include <physics/collider_system.h>

#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

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
	voe_physics_collider_system_run(world);
	voe_physics_body_system_run(world);
}

bool voe_game_frame(voe_app *app, voe_ecs_world *world,
		    const voe_3d_shapes *shapes, voe_base_arena *scratch,
		    voe_platform_size size, float lag)
{
	voe_render_device *device;
	voe_render_pass_camera camera;
	voe_3d_frame frame;
	bool drawing;
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
	camera = (voe_render_pass_camera){ frame.view, frame.light };
	passed = voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
				       &camera);
	if (passed) {
		voe_3d_draw_system_run(world, device, scratch, frame);
		voe_render_pass_end(device);
	}
	return voe_app_draw_close(app) && passed;
}
