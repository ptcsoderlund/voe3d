// The camera system: registration, creation, and the drain that is the only
// thing in the engine that moves a camera.
//
// THE CONSTANTS BELOW ARE WHAT "FASTER", "FURTHER" AND "MORE SENSITIVE" MEAN.
// They were cube.c's when the camera lived in render, and they are unchanged, so
// flying feels exactly as it did. A card that makes any of them settable is the
// card that decides where that setting lives; until then they are here, which is
// the folder that owns what a camera does.
//
// PLACEMENTS APPLY BEFORE MOTIONS, AND THAT ORDER IS WHY A HANDOVER DOES NOT
// JUMP. A frame that ends a scripted path and starts a hand on the keys submits
// a placement and a motion; applying the placement first means the hand starts
// from where the path had reached, which is exactly the seam card 015's orbit
// and card 016's keyboard needed and got with a special case at the time.
//
// PITCH IS CLAMPED SHORT OF STRAIGHT UP. At exactly straight up the up vector a
// look-at needs stops meaning anything and the view matrix goes to pieces, so
// the clamp is not a nicety.
//
// YAW IS WRAPPED AND PITCH IS NOT. Turning round and round is ordinary and a yaw
// that grows without bound loses precision; a pitch cannot go round at all
// because of the clamp above.
#include <base/assert.h>
#include <ecs/intent.h>
#include <scene/camera_system.h>

#include <math.h>

// A full turn, radians.
#define TURN 6.2831853f

// Metres a second, and what `fast` multiplies it by.
#define METRES_PER_SECOND 3.0f
#define FAST_MULTIPLIER 4.0f

// Radians per unit of whatever the window system calls a mouse delta. Small
// because the numbers platform hands out are large.
#define RADIANS_PER_UNIT 0.004f

// Just under a right angle: 89 degrees in radians. Straight up is where a
// look-at's up vector stops meaning anything.
#define PITCH_LIMIT 1.5533431f

static const struct voe_ecs_key placement_key = {
	"voe_scene_camera_placement"
};
static const struct voe_ecs_key motion_key = { "voe_scene_camera_motion" };

// World up, and it is world up rather than the camera's on purpose: looking at
// the floor and asking to rise still rises.
static voe_math_float3 world_up(void)
{
	return (voe_math_float3){ 0.0f, 1.0f, 0.0f };
}

// The camera's own right, kept horizontal. Taking it from the camera's tilted
// frame instead is what makes strafing while looking down sink into the floor.
static voe_math_float3 camera_right(float yaw)
{
	return (voe_math_float3){ cosf(yaw), 0.0f, -sinf(yaw) };
}

void voe_scene_camera_register(voe_ecs_world *world, uint32_t capacity)
{
	VOE_BASE_ASSERT(world != NULL, "registering cameras in no world");

	(void)voe_ecs_component_register(world, &voe_scene_camera_key,
					 sizeof(voe_scene_camera), capacity,
					 NULL);
	(void)voe_ecs_intent_register(world, &placement_key,
				      sizeof(voe_scene_camera_placement),
				      capacity);
	(void)voe_ecs_intent_register(world, &motion_key,
				      sizeof(voe_scene_camera_motion),
				      capacity);
}

bool voe_scene_camera_add(voe_ecs_world *world, voe_ecs_entity entity,
			  voe_scene_camera camera)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a camera to no world");
	VOE_BASE_DEBUG_ASSERT(camera.fov_y > 0.0f,
			      "a camera with no field of view");
	VOE_BASE_DEBUG_ASSERT(camera.near_plane > 0.0f,
			      "a camera whose near plane is at or behind the eye");
	VOE_BASE_DEBUG_ASSERT(camera.far_plane > camera.near_plane,
			      "a camera whose far plane is not further away than its near one");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_camera_key),
		entity, &camera);
}

bool voe_scene_camera_place(voe_ecs_world *world,
			    voe_scene_camera_placement placement)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "placing a camera in no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &placement_key), &placement);
}

bool voe_scene_camera_move(voe_ecs_world *world, voe_scene_camera_motion motion)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "moving a camera in no world");
	VOE_BASE_DEBUG_ASSERT(motion.seconds >= 0.0f,
			      "moving a camera backwards through time");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &motion_key), &motion);
}

static void apply_placement(voe_scene_camera *camera,
			    const voe_scene_camera_placement *placement)
{
	camera->eye = placement->eye;
	camera->yaw = placement->yaw;
	camera->pitch = placement->pitch;
}

static void apply_motion(voe_scene_camera *camera,
			 const voe_scene_camera_motion *motion)
{
	voe_math_float3 direction = { 0.0f, 0.0f, 0.0f };
	float speed = METRES_PER_SECOND;
	float length;

	// The mouse turns it. Both signs are subtractions: moving the mouse
	// right turns the camera right, which is a smaller yaw here because a
	// positive yaw turns left; moving it down looks down, and platform
	// reports +y as down.
	camera->yaw -= motion->look_x * RADIANS_PER_UNIT;
	camera->pitch -= motion->look_y * RADIANS_PER_UNIT;

	camera->yaw = fmodf(camera->yaw, TURN);
	camera->pitch = fmaxf(-PITCH_LIMIT, fminf(PITCH_LIMIT, camera->pitch));

	direction = voe_math_float3_add(
		direction,
		voe_math_float3_scale(voe_scene_camera_forward(*camera),
				      motion->forward));
	direction = voe_math_float3_add(
		direction, voe_math_float3_scale(camera_right(camera->yaw),
						 motion->right));
	direction = voe_math_float3_add(
		direction, voe_math_float3_scale(world_up(), motion->up));

	if (motion->fast)
		speed *= FAST_MULTIPLIER;

	// Normalized, so holding two keys is not faster than holding one — and
	// guarded, because asking for nothing is the ordinary case and dividing
	// by its length would be a division by zero.
	length = voe_math_float3_length(direction);
	if (length > 0.0f)
		camera->eye = voe_math_float3_add(
			camera->eye,
			voe_math_float3_scale(direction,
					      speed * motion->seconds / length));
}

void voe_scene_camera_system_run(voe_ecs_world *world)
{
	voe_ecs_type type;
	voe_ecs_intent placements;
	voe_ecs_intent motions;
	const voe_scene_camera_placement *placed;
	const voe_scene_camera_motion *moved;
	uint32_t count;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the camera system on no world");

	type = voe_ecs_component_type(world, &voe_scene_camera_key);
	placements = voe_ecs_intent_type(world, &placement_key);
	motions = voe_ecs_intent_type(world, &motion_key);

	// Read, change, write back. The component is copied out rather than
	// written through, because a table hands out const rows and the write is
	// a set — which is also what makes an intent naming a destroyed entity
	// fall out as a false the loop can ignore.
	placed = voe_ecs_intent_queue(world, placements);
	count = voe_ecs_intent_count(world, placements);
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_camera *row =
			voe_ecs_component_get(world, type, placed[i].entity);
		voe_scene_camera camera;

		if (row == NULL)
			continue;
		camera = *row;
		apply_placement(&camera, &placed[i]);
		(void)voe_ecs_component_set(world, type, placed[i].entity,
					    &camera);
	}

	moved = voe_ecs_intent_queue(world, motions);
	count = voe_ecs_intent_count(world, motions);
	for (uint32_t i = 0; i < count; i++) {
		const voe_scene_camera *row =
			voe_ecs_component_get(world, type, moved[i].entity);
		voe_scene_camera camera;

		if (row == NULL)
			continue;
		camera = *row;
		apply_motion(&camera, &moved[i]);
		(void)voe_ecs_component_set(world, type, moved[i].entity,
					    &camera);
	}

	voe_ecs_intent_clear(world, placements);
	voe_ecs_intent_clear(world, motions);
}
