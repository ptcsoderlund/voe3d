// The scroll's step: registers tank_scroll, queues it onto the scene's one
// camera with the lead, then moves the camera forward with the first hull and
// writes where the screen's bottom and top meet the hull's ground.
//
// The edges (tank_scroll.h): a centre ray in camera space is (0, ±tan(fov_y /
// 2), -1), turned into the world by the camera's rotation; it meets y = the
// hull's y at t = (hull y - eye y) / ray y when that t is positive and finite,
// at eye z + t ray z. Else it never falls to the ground: eye z less far_plane.
// The pose is the one this step moves the camera to, so the edges match the
// frame drawn; the lens is the camera row's current fov_y, fitted or not.
//
// Constraints: the runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). The camera's scale
// is not applied to the rays (a scaled camera sees its ground unscaled here).
// A full structural or transform queue tries again next step.
#include "tank_scroll.h"
#include "tank_hull.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <scene/camera_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <math.h>

const struct voe_ecs_key tank_scroll_key = { "tank_scroll" };

bool tank_scroll_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the scroll in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_scroll_key, sizeof(tank_scroll), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

const tank_scroll *tank_scroll_get(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "reading the scroll in no world");
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_scroll_key);

	if (voe_ecs_component_count(world, type) == 0)
		return NULL;
	return voe_ecs_component_rows(world, type);
}

// The z where the centre ray at `slope` (tan of the half field, signed: + up)
// from `pose` meets y = `ground`, or the eye's z less `far_plane` when it
// never does.
static double tank_scroll_edge(voe_scene_transform pose, double slope,
			       double ground, double far_plane)
{
	VOE_BASE_ASSERT(isfinite(slope) && far_plane > 0.0,
			"an edge from no lens");
	const voe_math_quat q = pose.rotation;
	// The rotation's +Y and +Z columns, y and z only: ray = up slope - back.
	const double up_y = 1.0 - 2.0 * ((double)q.x * q.x + (double)q.z * q.z);
	const double up_z = 2.0 * ((double)q.y * q.z + (double)q.w * q.x);
	const double back_y = 2.0 * ((double)q.y * q.z - (double)q.w * q.x);
	const double back_z = 1.0 - 2.0 * ((double)q.x * q.x + (double)q.y * q.y);
	const double ray_y = up_y * slope - back_y;
	const double ray_z = up_z * slope - back_z;
	const double t = (ground - pose.position.y) / ray_y;

	if (!isfinite(t) || t <= 0.0)
		return pose.position.z - far_plane;
	VOE_BASE_DEBUG_ASSERT(isfinite(pose.position.z + t * ray_z),
			      "an edge past every number");
	return pose.position.z + t * ray_z;
}

// The row whole for a camera at `pose` with `lens` over a hull at `ground`.
static tank_scroll tank_scroll_row(double lead, voe_scene_transform pose,
				   voe_scene_camera lens, double ground)
{
	VOE_BASE_ASSERT(isfinite(lead), "a scroll with no lead");
	const double slope = tan(0.5 * (double)lens.fov_y);
	const tank_scroll row = {
		.lead = lead,
		.bottom = tank_scroll_edge(pose, -slope, ground, lens.far_plane),
		.top = tank_scroll_edge(pose, slope, ground, lens.far_plane),
	};

	return row;
}

void tank_scroll_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"running the scroll in no world");
	voe_ecs_world *world = step->world;
	const voe_ecs_type hull_type =
		voe_ecs_component_type(world, &tank_hull_key);

	if (voe_scene_camera_count(world) == 0 ||
	    voe_ecs_component_count(world, hull_type) == 0)
		return;
	const voe_ecs_entity camera = voe_scene_camera_entities(world)[0];
	const voe_ecs_entity hull =
		voe_ecs_component_entities(world, hull_type)[0];
	const voe_scene_transform *eye = voe_scene_transform_get(world, camera);

	if (eye == NULL || voe_scene_transform_get(world, hull) == NULL)
		return;
	const voe_math_double3 tank =
		voe_scene_transform_world(world, hull).position;
	const voe_scene_camera lens = voe_scene_camera_rows(world)[0];
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_scroll_key);
	const tank_scroll *now = voe_ecs_component_get(world, type, camera);

	if (now == NULL) {
		const double lead = eye->position.z - tank.z;
		const tank_scroll first = tank_scroll_row(lead, *eye, lens, tank.y);

		(void)voe_ecs_structure_add(world, type, camera, &first);
		return;
	}
	voe_scene_transform pose = *eye;

	if (tank.z + now->lead < pose.position.z) {
		pose.position.z = tank.z + now->lead;
		if (!voe_scene_transform_submit(world, (voe_scene_transform_intent){
				camera, pose }))
			pose = *eye;
	}
	const tank_scroll row = tank_scroll_row(now->lead, pose, lens, tank.y);
	const bool written = voe_ecs_component_set(world, type, camera, &row);

	VOE_BASE_ASSERT(written, "the scroll row went missing");
	VOE_BASE_DEBUG_ASSERT(pose.position.z <= eye->position.z,
			      "the camera went back");
}
