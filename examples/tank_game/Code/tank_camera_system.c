// The tank camera's step: registers tank_camera_fit, adds it to the scene's
// one camera holding the authored fov_y, then fits the lens to the window.
//
// The fit (tank_camera.h): aspect a = width / height; at a >= 16/9 the
// authored fov_y, else 2 atan(tan(authored / 2) (16/9) / a), capped at 3.0
// rad. Near and far are kept; the whole lens is submitted (scene's
// camera_system.h), and only when it differs from the current one.
//
// Constraints: the runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). A full structural
// or camera queue tries again next step.
#include "tank_camera.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <platform/window.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>

#include <math.h>

#define TANK_CAMERA_ASPECT (16.0f / 9.0f)
#define TANK_CAMERA_FOV_MAX 3.0f

const struct voe_ecs_key tank_camera_fit_key = { "tank_camera_fit" };

bool tank_camera_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the tank camera in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_camera_fit_key, sizeof(tank_camera_fit), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

// The fov_y that keeps the horizontal field at 16:9 for aspect `a`.
static float tank_camera_fitted(float authored, float a)
{
	VOE_BASE_ASSERT(a > 0.0f && authored > 0.0f,
			"fitting a lens to no aspect");
	if (a >= TANK_CAMERA_ASPECT)
		return authored;
	const float wide =
		2.0f * atanf(tanf(authored * 0.5f) * TANK_CAMERA_ASPECT / a);
	const float fov = fminf(wide, TANK_CAMERA_FOV_MAX);

	VOE_BASE_DEBUG_ASSERT(fov >= authored, "a fitted lens narrower");
	return fov;
}

void tank_camera_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"fitting the tank camera in no world");
	voe_ecs_world *world = step->world;

	if (step->window == NULL || voe_scene_camera_count(world) == 0)
		return;
	const voe_ecs_entity camera = voe_scene_camera_entities(world)[0];
	const voe_scene_camera lens = voe_scene_camera_rows(world)[0];
	const voe_ecs_type type =
		voe_ecs_component_type(world, &tank_camera_fit_key);
	const tank_camera_fit *fit = voe_ecs_component_get(world, type, camera);

	if (fit == NULL) {
		const tank_camera_fit first = { lens.fov_y };

		(void)voe_ecs_structure_add(world, type, camera, &first);
		return;
	}
	const voe_platform_size size = voe_platform_window_size(step->window);

	if (size.width <= 0 || size.height <= 0)
		return;
	voe_scene_camera wanted = lens;

	wanted.fov_y = tank_camera_fitted(
		fit->fov_y, (float)size.width / (float)size.height);
	VOE_BASE_DEBUG_ASSERT(wanted.fov_y <= TANK_CAMERA_FOV_MAX ||
				      wanted.fov_y == fit->fov_y,
			      "a fitted lens past the cap");
	if (wanted.fov_y != lens.fov_y)
		(void)voe_scene_camera_submit(world, (voe_scene_camera_intent){
			camera, wanted });
}
