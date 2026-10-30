// A fog volume: a box of fog placed in the world, and how far the wind has
// moved its patches (0480 point 4, 0493 points 1 and 2). Read by anyone,
// const; written only through scene/fog_volume_system.h.
//
// THE BOX IS PLACED, TURNED AND SCALED BY ITS TRANSFORM, as a light blocker's
// is, so a parent carries it and the gizmos edit it with no case of their own.
// FOG IS ONLY INSIDE IT, fading softly towards its faces so no box shape
// shows; outside every volume there is no fog, only haze. A world holds many.
//
// AN OFF ENTITY'S VOLUME IS NO FOG (0427). It draws, casts and collides with
// nothing: it is not a mesh, a shadow caster or a collider.
//
// THE FIELDS, every number finite:
//   size             — metres along the box's local axes before scale, the
//                      whole edge and not a half, each at least 0.
//   thickness        — 0 to 1: 0 no fog, 1 the thickest.
//   patchiness       — 0 to 1: 0 even fog, 1 patches with clear air between.
//   wind_direction   — degrees, any finite, the way the wind blows towards:
//                      0 towards -Z, 90 towards +X, as the clouds'.
//   wind_speed       — metres a second, at least 0.
//   godrays_strength — 0 to 1, how strongly the sun's beams show in it; at 0
//                      the fog is lit evenly. The fog stays either way.
//
// THE DRIFT, voe_scene_fog_volume_drift, IS HOW FAR ITS PATCHES HAVE MOVED:
// metres along x and z, in double, one per volume, runtime-only and never
// saved. The fog volume system adds it, moves it and drops it with the row.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_fog_volume_description(),
// and one that does not has the same struct and nothing more. Every field is
// authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// Bounds per field — see the header.
#define VOE_SCENE_FOG_VOLUME_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, size, FLOAT3)            \
	F(float, thickness, FLOAT32)                \
	F(float, patchiness, FLOAT32)               \
	F(float, wind_direction, FLOAT32)           \
	F(float, wind_speed, FLOAT32)               \
	F(float, godrays_strength, FLOAT32)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_fog_volume, VOE_SCENE_FOG_VOLUME_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_fog_volume_key;

// NULL when the entity has no fog volume, or is not alive any more. The
// pointer is into the table and is good until the next add or remove.
const voe_scene_fog_volume *
voe_scene_fog_volume_get(const voe_ecs_world *world, voe_ecs_entity entity);

// The table, for the system that hands the volumes to the GPU. rows[i]
// belongs to entities[i], and both are `count` long.
uint32_t voe_scene_fog_volume_count(const voe_ecs_world *world);
const voe_scene_fog_volume *
voe_scene_fog_volume_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_fog_volume_entities(const voe_ecs_world *world);

// Metres the patches have moved on x and z since the row's drift was added.
typedef struct {
	double x;
	double z;
} voe_scene_fog_volume_drift;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_fog_volume_drift_key;

// NULL when the entity has no drift yet, or is not alive.
const voe_scene_fog_volume_drift *
voe_scene_fog_volume_drift_get(const voe_ecs_world *world,
			       voe_ecs_entity entity);
