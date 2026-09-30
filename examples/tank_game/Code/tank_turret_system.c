// The tank turret system: the control row (tank_control.h) into the aim of
// the player's turrets, those within the control row's hull, from the mouse
// pointer or the pad's right stick. Teaches turning a child in the world: the
// aim is found in the world, and what is written is the turret's own row,
// relative to the hull it sits on (0271). The turn itself is
// tank_turret_turn_toward, which any owner calls for its own turret (0297).
//
// Without `pad`, the ray is the world's first camera's, through the pointer,
// and the aim is where it crosses the level plane through the turret's world
// position: the plane is where a top-down aim means anything, and at the
// turret's own height the aim is never above or below it, so only the turn
// about +Y is left. A ray parallel to that plane, or pointing away from it,
// aims nowhere and the turret stays. With `pad`, the camera's +X and -Z
// flattened onto the ground and normalised are screen right and up, and the
// aim is the turret's world position plus right × `aim_x` + up × `aim_y`;
// with no `aim` the turret holds. The turret's world rotation is turned about +Y
// toward facing the aim with its barrel, its -Z turned by `aim` about its own
// +Y, by at most `turn` a second, and
// voe_scene_transform_local makes that its row under the hull. A barrel
// under the turret follows it with no code here, by parenting.
//
// Constraints: the camera read is this step's, not the drawn frame's
// interpolated one; the hull's turn this step lands after the aim is read,
// so a turning tank's turret lags it by one step. Nothing with no control
// row (headless, or before its first step) or no camera; on the pointer,
// nothing with no pointer over the window or a view that sees nothing. A
// camera looking straight down has no flat -Z, so its +Y is screen up. A
// full transform queue leaves the rest of the turrets where they were.
#include "tank_control.h"
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
#include <scene/parent_component.h>
#include <scene/transform_component.h>
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
		&tank_turret_key, sizeof(tank_turret), VOE_GAME_WORLD_MAX_DRAWN,
		VOE_GAME_PROJECT_DESCRIPTION(tank_turret), &tank_turret_default,
		"Tank / Turret" }) &&
	       voe_game_project_component_needs(world, &tank_turret_key,
						&voe_scene_transform_key);
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

typedef struct {
	float x;
	float z;
} tank_turret_ground;

// (x, z) normalised. False when it is too short to have a direction.
static bool flat_unit(float x, float z, tank_turret_ground *out)
{
	VOE_BASE_ASSERT(out != NULL, "flattening into nothing");
	const float length = sqrtf(x * x + z * z);

	if (!(length > 1e-3f))
		return false;
	*out = (tank_turret_ground){ x / length, z / length };
	VOE_BASE_DEBUG_ASSERT(fabsf(out->x) <= 1.01f && fabsf(out->z) <= 1.01f,
			      "a unit longer than one");
	return true;
}

// The pad's aim as a ground offset from the turret: the stick's right and up
// along the first camera's +X and -Z (+Y when -Z is straight down),
// flattened. False with no aim, no camera with a transform, or a camera
// whose right is straight up or down.
static bool pad_aim(const voe_ecs_world *world, const tank_control *control,
		    tank_turret_ground *out)
{
	VOE_BASE_ASSERT(world != NULL && control != NULL && out != NULL,
			"a pad aim from nothing");
	if ((control->aim_x == 0.0f && control->aim_y == 0.0f) ||
	    voe_scene_camera_count(world) == 0)
		return false;
	const voe_ecs_entity camera = voe_scene_camera_entities(world)[0];

	if (voe_scene_transform_get(world, camera) == NULL)
		return false;
	const voe_math_quat q = voe_scene_transform_world(world, camera).rotation;
	tank_turret_ground right;
	tank_turret_ground up;

	// The matrix's first column, its third negated, and its second.
	if (!flat_unit(1.0f - 2.0f * (q.y * q.y + q.z * q.z),
		       2.0f * (q.x * q.z - q.w * q.y), &right) ||
	    (!flat_unit(-2.0f * (q.x * q.z + q.w * q.y),
			-(1.0f - 2.0f * (q.x * q.x + q.y * q.y)), &up) &&
	     !flat_unit(2.0f * (q.x * q.y - q.w * q.z),
			2.0f * (q.y * q.z + q.w * q.x), &up)))
		return false;
	*out = (tank_turret_ground){
		right.x * control->aim_x + up.x * control->aim_y,
		right.z * control->aim_x + up.z * control->aim_y,
	};
	VOE_BASE_DEBUG_ASSERT(isfinite(out->x) && isfinite(out->z),
			      "a pad aim going nowhere");
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
// offset applied first, so its -Z is the barrel, not the bare -Z. `left` is
// the radians still to turn after it, at or above 0. False when that forward
// is straight up or down and has no heading to turn.
static bool turn_toward(voe_math_quat q, float x, float z, float limit,
			float *out, float *left)
{
	VOE_BASE_ASSERT(out != NULL && left != NULL && limit >= 0.0f,
			"a turn into nothing");
	// (0, 0, -1) rotated by q: the third column of its matrix, negated.
	const float fx = -2.0f * (q.x * q.z + q.w * q.y);
	const float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));

	if (fx * fx + fz * fz < 1e-6f)
		return false;
	const float wanted = remainderf(yaw_toward(x, z) - yaw_toward(fx, fz),
					TANK_TURRET_TWO_PI);

	*out = fmaxf(-limit, fminf(limit, wanted));
	*left = fabsf(wanted - *out);
	VOE_BASE_DEBUG_ASSERT(fabsf(*out) <= limit, "a turn past its limit");
	return true;
}

bool tank_turret_turn_toward(voe_ecs_world *world, voe_ecs_entity turret,
			     voe_math_double3 at, double seconds,
			     voe_math_quat *barrel, float *left)
{
	VOE_BASE_ASSERT(world != NULL && barrel != NULL && left != NULL,
			"turning a turret into nothing");
	VOE_BASE_ASSERT(seconds >= 0.0, "turning a turret back in time");
	const tank_turret *found = voe_ecs_component_get(
		world, voe_ecs_component_type(world, &tank_turret_key), turret);

	if (found == NULL || voe_scene_transform_get(world, turret) == NULL)
		return false;
	const tank_turret row = *found;
	voe_scene_transform placed = voe_scene_transform_world(world, turret);
	// The barrel: -Z turned by the aim offset about the turret's own +Y.
	const voe_math_quat aim = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 1.0f, 0.0f },
		row.aim * TANK_TURRET_RADIANS_PER_DEGREE);
	const double x = at.x - placed.position.x;
	const double z = at.z - placed.position.z;
	float turn;
	float remaining;

	if (x * x + z * z < 1e-6 ||
	    !turn_toward(voe_math_quat_mul(placed.rotation, aim), (float)x,
			 (float)z,
			 row.turn * TANK_TURRET_RADIANS_PER_DEGREE *
				 (float)seconds,
			 &turn, &remaining))
		return false;
	placed.rotation = voe_math_quat_normalize(voe_math_quat_mul(
		voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, turn),
		placed.rotation));
	if (!voe_scene_transform_submit(
		    world, (voe_scene_transform_intent){
				   turret, voe_scene_transform_local(
						   world, turret, placed) }))
		return false;
	*barrel = voe_math_quat_mul(placed.rotation, aim);
	*left = remaining / TANK_TURRET_RADIANS_PER_DEGREE;
	VOE_BASE_DEBUG_ASSERT(*left >= 0.0f, "a turn left below nought");
	return true;
}

void tank_turret_system_run(voe_ecs_world *world, voe_platform_window *window,
			    double seconds)
{
	VOE_BASE_ASSERT(world != NULL, "aiming turrets in no world");
	VOE_BASE_ASSERT(seconds >= 0.0, "aiming turrets back in time");
	const voe_ecs_type control_type =
		voe_ecs_component_type(world, &tank_control_key);

	if (voe_ecs_component_count(world, control_type) == 0)
		return;
	const tank_control *control =
		voe_ecs_component_rows(world, control_type);
	voe_3d_ray ray = { 0 };
	tank_turret_ground offset = { 0 };

	if (control->pad) {
		if (!pad_aim(world, control, &offset))
			return;
	} else if (window == NULL || !pointer_ray(world, window, &ray) ||
		   fabsf(ray.direction.y) < 1e-6f) {
		// A ray parallel to every level plane aims nowhere.
		return;
	}
	const voe_ecs_type type = voe_ecs_component_type(world, &tank_turret_key);
	const voe_ecs_entity hull =
		voe_ecs_component_entities(world, control_type)[0];
	const voe_ecs_entity *entities = voe_ecs_component_entities(world, type);
	const uint32_t count = voe_ecs_component_count(world, type);

	VOE_BASE_ASSERT(count <= VOE_GAME_WORLD_MAX_DRAWN,
			"more tank turret rows than were registered");
	for (uint32_t i = 0; i < count; i++) {
		if (!voe_scene_parent_within(world, entities[i], hull) ||
		    voe_scene_transform_get(world, entities[i]) == NULL)
			continue;
		const voe_scene_transform placed =
			voe_scene_transform_world(world, entities[i]);
		voe_math_double3 at = { placed.position.x + offset.x,
					placed.position.y,
					placed.position.z + offset.z };

		if (!control->pad) {
			const double along = (placed.position.y - ray.origin.y) /
					     (double)ray.direction.y;

			// Pointing away from the plane: no aim.
			if (along <= 0.0)
				continue;
			at.x = ray.origin.x + ray.direction.x * along;
			at.z = ray.origin.z + ray.direction.z * along;
		}
		voe_math_quat barrel;
		float left;

		// Nothing turned leaves this turret where it was.
		(void)tank_turret_turn_toward(world, entities[i], at, seconds,
					      &barrel, &left);
	}
}
