// The only way a camera moves: one of two intents, drained by this system.
//
//     voe_scene_camera_register(world, 4);
//     voe_scene_camera_add(world, eye_entity, (voe_scene_camera){ ... });
//
//     // put it somewhere outright — a scripted path, a cut, a load:
//     voe_scene_camera_place(world, (voe_scene_camera_placement){
//             .entity = eye_entity, .eye = where, .yaw = y, .pitch = p });
//
//     // or move it by what a person did since the last frame:
//     voe_scene_camera_move(world, (voe_scene_camera_motion){
//             .entity = eye_entity, .forward = 1.0f, .look_x = dx,
//             .seconds = dt });
//
//     voe_scene_camera_system_run(world);
//
// TWO INTENTS, BECAUSE THERE ARE TWO DIFFERENT THINGS TO SAY. A placement is an
// absolute answer and a motion is a relative one, and squeezing them into one
// type would mean a field that is sometimes a position and sometimes a speed.
// Within a queue, intents apply in submission order; between the two queues,
// every placement applies before every motion in the same run. That order is
// stated rather than discovered because it is the one a handover needs: put the
// camera where the path had got to, then let the hand move it from there.
//
// A MOTION IS A DIRECTION AND A DURATION, NOT A DISTANCE. forward, right and up
// are -1 to 1 in the camera's own frame — except up, which is world up, so
// looking at the floor and asking to go up still goes up. How fast that is is
// this system's constant; how long the frame was is the caller's `seconds`,
// because nothing in the engine can measure a frame yet and the frame loop is
// the one place that knows what it assumed. A diagonal is not faster than a
// straight line: the direction is normalized before the speed is applied.
//
// look_x AND look_y ARE THE MOUSE'S OWN NUMBERS, UNSCALED. platform hands out a
// delta whose scale is the window system's; turning that into an angle needs a
// sensitivity, and the sensitivity belongs with the camera. +x is right and +y
// is down, which is what platform reports.
//
// `fast` IS A MULTIPLIER'S WORTH AND THE MULTIPLIER IS THIS SYSTEM'S. What key
// means "faster" is a binding and belongs at a call site; what faster does is
// here.
//
// IT SETS NO MENU PATH, SO ADD COMPONENT NEVER OFFERS A CAMERA: a scene has
// exactly one, made with it (0218), and a type without a path is not offered
// (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <math/float3.h>
#include <scene/camera_component.h>

#include <stdint.h>

// Registers the table, both intent queues and the default row: at the origin,
// looking down -Z, 60° of field of view, planes at 0.1 and 1000. The default row
// is what "add at default" gives (0190). capacity is how many cameras the
// world may hold, and how many of each intent may be waiting.
void voe_scene_camera_register(voe_ecs_world *world, uint32_t capacity);

// False when the table is full or the entity is not alive.
[[nodiscard]] bool voe_scene_camera_add(voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_scene_camera camera);

// Put the camera exactly here, looking exactly this way. The lens — field of
// view and the two planes — is not touched.
typedef struct {
	voe_ecs_entity entity;
	voe_math_float3 eye;
	float yaw;
	float pitch;
} voe_scene_camera_placement;

// Move and turn it by what happened over `seconds`.
typedef struct {
	voe_ecs_entity entity;
	float forward;
	float right;
	float up;
	float look_x;
	float look_y;
	bool fast;
	float seconds;
} voe_scene_camera_motion;

// Both false only when their queue is full.
[[nodiscard]] bool voe_scene_camera_place(voe_ecs_world *world,
					  voe_scene_camera_placement placement);
[[nodiscard]] bool voe_scene_camera_move(voe_ecs_world *world,
					 voe_scene_camera_motion motion);

// Applies every placement, then every motion, then empties both queues.
void voe_scene_camera_system_run(voe_ecs_world *world);
