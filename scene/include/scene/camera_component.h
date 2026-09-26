// What a camera can see: a vertical field of view and the two planes. The lens
// only — the numbers a person would author, and nothing a graphics API would
// recognise.
//
// WHERE THE CAMERA IS AND HOW IT FACES IS ITS ENTITY'S TRANSFORM (0222). A camera
// is a thing in 3D space like any other, so it has one pose, in the transform
// component, and is moved as anything else is. The camera component needs a
// transform on the same entity; the view below is built from that transform.
//
// THE VIEW IS THE PLAIN INVERSE OF THE TRANSFORM'S MATRIX, SCALE AND ROLL
// INCLUDED. Nothing is stripped: a rolled camera films tilted, and a scaled one
// sees the world scaled the other way, as a child of that transform would. How a
// project is structured is its developer's call, not a camera-only exception
// here. A pose whose matrix has no inverse (a scale of nought on an axis) sees
// nothing: the view call returns false rather than asserting (0223), so a typed
// scale cannot stop a program.
//
// THE PROJECTION MATRIX IS NOT BUILT HERE AND MUST NOT BE. Clip space, the
// reversed depth this engine uses and the Y flip that goes with it are all
// `render`'s and `3d`'s business (CLAUDE.md, Conventions), and `math` is not
// allowed to know about them either. So this holds a field of view in radians
// and two distances in metres, and `3d` turns those into sixteen floats. The
// view matrix below is a different matter: it is where the camera is and which
// way it faces, which is scene's own.
//
// THE VIEW SEES THE WORLD RELATIVE TO ITS OWN EYE (ADR-0250). A point is taken
// into it as its position minus the eye, in double, before this matrix, so the
// view itself carries no translation and nothing 100 km out loses precision.
//
// A CAMERA LOOKS ALONG ITS OWN -Z (CLAUDE.md), so an unrotated pose looks down
// the world's -Z with +Y up.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_camera_description(), and one
// that does not has the same struct and nothing more. Every field is authored and
// none is read-only; the lens is edited through camera_system.h's intent.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float4x4.h>

#include <scene/transform_component.h>

#include <stdint.h>

// fov_y is the vertical field of view, radians; the horizontal one falls out of
// the aspect ratio, which is the render target's and not the camera's.
// near_plane and far_plane are metres, and spelled out because `near` and `far`
// are macros in a Windows header, and a struct field that happens to collide
// with one is a mystery to whoever hits it.
#define VOE_SCENE_CAMERA_FIELDS(F, F_READ_ONLY) \
	F(float, fov_y, FLOAT32)                \
	F(float, near_plane, FLOAT32)           \
	F(float, far_plane, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_camera, VOE_SCENE_CAMERA_FIELDS)

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_camera_key;

// The eye-relative world-to-camera matrix for a camera at `pose`: the inverse of
// voe_scene_transform_matrix(pose, pose.position), so it has no translation. No
// projection in it and no flip. False, with `out` untouched, when that matrix
// has no inverse.
[[nodiscard]] bool voe_scene_camera_view(voe_scene_transform pose,
					 voe_math_float4x4 *out);

// NULL when the entity has no camera or is not alive.
const voe_scene_camera *voe_scene_camera_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

uint32_t voe_scene_camera_count(const voe_ecs_world *world);
const voe_scene_camera *voe_scene_camera_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_camera_entities(const voe_ecs_world *world);
