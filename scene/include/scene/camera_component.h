// Where a camera is and what it can see: a position, two angles, a vertical
// field of view and the two planes. The numbers a person would author, and
// nothing a graphics API would recognise.
//
// THE PROJECTION MATRIX IS NOT BUILT HERE AND MUST NOT BE. Clip space, the
// reversed depth this engine uses and the Y flip that goes with it are all
// `render`'s and `3d`'s business (CLAUDE.md, Conventions), and `math` is not
// allowed to know about them either. So this holds a field of view in radians
// and two distances in metres, and `3d` turns those into sixteen floats. The
// view matrix below is a different matter: it is where the camera is and which
// way it faces, which is scene's own.
//
// ORIENTATION IS TWO ANGLES AND NOT A QUATERNION, WHICH IS A CAMERA-SHAPED
// CHOICE. A camera that can be flown has to stop short of straight up, and a
// clamp is a comparison on a pitch and an awkward question to ask of a
// quaternion. There is no roll, and nothing has wanted one: the day something
// does, this is the line that changes and everything reading the view matrix
// stays as it is.
//
// A CAMERA LOOKS ALONG ITS OWN -Z (CLAUDE.md). Zero yaw looks along -Z, a
// positive yaw turns towards -X — a left turn, which is where a positive
// rotation about +Y goes in this engine — and a positive pitch looks up.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_camera_description(), and one
// that does not has the same struct and nothing more. Every field is authored and
// none is read-only; there is no replace intent, so a tool shows a camera and does
// not edit it.
#pragma once

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>
#include <math/float4x4.h>

#include <stdint.h>

// yaw and pitch are radians. fov_y is the vertical field of view, radians; the
// horizontal one falls out of the aspect ratio, which is the render target's and
// not the camera's. near_plane and far_plane are metres, and spelled out because
// `near` and `far` are macros in a Windows header, and a struct field that happens
// to collide with one is a mystery to whoever hits it.
#define VOE_SCENE_CAMERA_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, eye, FLOAT3)         \
	F(float, yaw, FLOAT32)                  \
	F(float, pitch, FLOAT32)                \
	F(float, fov_y, FLOAT32)                \
	F(float, near_plane, FLOAT32)           \
	F(float, far_plane, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_camera, VOE_SCENE_CAMERA_FIELDS)

extern const struct voe_ecs_key voe_scene_camera_key;

// The unit vector the camera is looking along. Public because the view matrix is
// built from it and because a caller that wants to know where a camera points
// should not be doing trigonometry of its own.
voe_math_float3 voe_scene_camera_forward(voe_scene_camera camera);

// The world-to-camera matrix: right-handed, +Y up, looking along -Z. No
// projection in it and no flip.
voe_math_float4x4 voe_scene_camera_view(voe_scene_camera camera);

// NULL when the entity has no camera or is not alive.
const voe_scene_camera *voe_scene_camera_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

uint32_t voe_scene_camera_count(const voe_ecs_world *world);
const voe_scene_camera *voe_scene_camera_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_camera_entities(const voe_ecs_world *world);
