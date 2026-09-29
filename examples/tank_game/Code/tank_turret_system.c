// The tank turret system: the mouse pointer into a turret's aim. Teaches
// turning a child in the world: the aim is found in the world, and what is
// written is the turret's own row, relative to the hull it sits on (0271).
//
// The ray is the world's first camera's, through the pointer, and the aim is
// where it crosses the level plane through the turret's world position:
// the plane is where a top-down aim means anything, and at the turret's own
// height the aim is never above or below it, so only the turn about +Y is
// left. A ray parallel to that plane, or pointing away from it, aims nowhere
// and the turret stays. The turret's world rotation is turned about +Y
// toward facing the aim with its barrel, its -Z turned by `aim` about its own
// +Y, by at most `turn` a second, and
// voe_scene_transform_local makes that its row under the hull. A barrel
// under the turret follows it with no code here, by parenting.
//
// Constraints: the camera read is this step's, not the drawn frame's
// interpolated one; the hull's turn this step lands after the aim is read,
// so a turning tank's turret lags it by one step. Nothing with no window
// (headless), no pointer over it, no camera, or a view that sees nothing.
// A full transform queue leaves the rest of the turrets where they were.
#include "tank_turret.h"

#include <3d/pick.h>
#include <3d/projection.h>

#include <base/assert.h>

#include <ecs/component.h>

#include <game/project.h>
#include <game/world.h>

#include <math/quat.h>

#include <platform/input.h>

#include <scene/camera_component.h>
#include <scene/transform_system.h>

#include <math.h>

#define TANK_TURRET_RADIANS_PER_DEGREE (3.14159265358979323846f / 180.0f)
#define TANK_TURRET_TWO_PI (2.0f * 3.14159265358979323846f)

const struct voe_ecs_key tank_turret_key = { "tank_turret" };

static const tank_turret tank_turret_default = { .turn = 180.0f,
						      .aim = 0.0f };

bool tank_turret_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering tank turret in no world");
	return voe_game_project_component(world, &(voe_game_project_type){
		&tank_turret_key, sizeof(tank_turret), VOE_GAME_WORLD_AUTHORED,
		VOE_GAME_PROJECT_DESCRIPTION(tank_turret), &tank_turret_default,
		"Tank / Turret" });
}

// The first camera's ray through the pointer. False when there is no
// pointer over the window, no camera with a transform, no picture, or a
// view that sees nothing.
static bool pointer_ray(const voe_ecs_world *world,
			voe_platform_window *window, voe_3d_ray *out)
{
	VOE_BASE_ASSERT(world != NULL && window != NULL && out != NULL,
			"a pointer ray from nothing");
	const voe_platform_pointer pointer = voe_platform_input_pointer(window);
	const voe_platform_size size = voe_platform_window_size(window);

	if (!pointer.over || size.width <= 0 || size.height <= 0 ||
	    voe_scene_camera_count(world) == 0)
		return false;
	const voe_ecs_entity camera = voe_scene_camera_entities(world)[0];

	if (voe_scene_transform_get(world, camera) == NULL)
		return false;
	const voe_scene_transform pose = voe_scene_transform_world(world, camera);
	voe_render_view view;

	if (!voe_3d_view(pose, voe_scene_camera_rows(world)[0],
			 (float)size.width / (float)size.height, &view))
		return false;
	*out = voe_3d_pick_ray(view, pose.position, size,
			       (voe_math_float2){ pointer.x, pointer.y });
	VOE_BASE_DEBUG_ASSERT(isfinite(out->direction.x), "a ray going nowhere");
	return true;
}

// The yaw about +Y that turns -Z to face (x, z): the inverse of
// voe_math_quat_from_axis_angle about +Y applied to (0, 0, -1).
static float yaw_toward(float x, float z)
{
	VOE_BASE_DEBUG_ASSERT(isfinite(x) && isfinite(z), "a yaw toward nowhere");
	const float yaw = atan2f(-x, -z);

	VOE_BASE_DEBUG_ASSERT(fabsf(yaw) <= 3.1416f, "a yaw past a half turn");
	return yaw;
}

// The turn about +Y, at most `limit` radians either way, that faces the
// barrel toward the aim `(x, z)` away. `q` is the world rotation with the aim
// offset applied first, so its -Z is the barrel, not the bare -Z. False when
// that forward is straight up or down and has no heading to turn.
static bool turn_toward(voe_math_quat q, float x, float z, float limit,
			float *out)
{
	VOE_BASE_ASSERT(out != NULL && limit >= 0.0f, "a turn into nothing");
	// (0, 0, -1) rotated by q: the third column of its matrix, negated.
	const float fx = -2.0f * (q.x * q.z + q.w * q.y);
	const float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));

	if (fx * fx + fz * fz < 1e-6f)
		return false;
	const float wanted = remainderf(yaw_toward(x, z) - yaw_toward(fx, fz),
					TANK_TURRET_TWO_PI);

	*out = fmaxf(-limit, fminf(limit, wanted));
	VOE_BASE_DEBUG_ASSERT(fabsf(*out) <= limit, "a turn past its limit");
	return true;
}

void tank_turret_system_run(voe_ecs_world *world, voe_platform_window *window,
			    double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "aiming turrets in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "aiming turrets back in time");
	voe_3d_ray ray;

	// A ray parallel to every level plane aims nowhere.
	if (window == NULL || !pointer_ray(world, window, &ray) ||
	    fabsf(ray.direction.y) < 1e-6f)
		return;
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_turret_key);
	const tank_turret *rows = voe_ecs_component_rows(world, type);
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_AUTHORED,
			"more tank turret rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		if (voe_scene_transform_get(world, entities[i]) == NULL)
			continue;
		voe_scene_transform placed =
			voe_scene_transform_world(world, entities[i]);
		const double along = (placed.position.y - ray.origin.y) /
				     (double)ray.direction.y;
		// The barrel: -Z turned by the aim offset about the turret's own +Y.
		const voe_math_quat barrel = voe_math_quat_mul(
			placed.rotation,
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f },
				rows[i].aim * TANK_TURRET_RADIANS_PER_DEGREE));
		float turn;

		// Pointing away from the plane: no aim.
		if (along <= 0.0)
			continue;
		const double x = ray.origin.x + ray.direction.x * along -
				 placed.position.x;
		const double z = ray.origin.z + ray.direction.z * along -
				 placed.position.z;

		if (x * x + z * z < 1e-6 ||
		    !turn_toward(barrel, (float)x, (float)z,
				 rows[i].turn * TANK_TURRET_RADIANS_PER_DEGREE *
					 (float)seconds,
				 &turn))
			continue;
		placed.rotation = voe_math_quat_normalize(voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				(voe_math_float3){ 0.0f, 1.0f, 0.0f }, turn),
			placed.rotation));
		if (!voe_scene_transform_submit(world,
						(voe_scene_transform_intent){
							entities[i],
							voe_scene_transform_local(
								world,
								entities[i],
								placed) }))
			return;
	}
}
