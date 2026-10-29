// The clouds: a layer over the world, Flat or Volumetric, and how far the
// wind has moved it (0480, 0492 points 1 and 2). Read by anyone, const;
// written only through scene/clouds_system.h.
//
// HAVING THE ROW IS CLOUDS ON. No row on, no clouds and no cloud shadow.
//
// THE FIRST ROW IN TABLE ORDER WHOSE ENTITY IS ON IS USED (0440); an off row
// is as no row (0427). A later row on, if a caller allows one, is ignored.
//
// THE FIELDS, every number finite:
//   kind            — Flat, a layer at one height, or Volumetric, a layer with
//                     depth that can be flown into. A UINT32 named by
//                     voe_scene_clouds_kind_names (0198).
//   coverage        — 0 to 1: 0 no cloud anywhere, 1 cloud over every point.
//   height          — the layer's bottom, world metres, y up; any finite.
//   thickness       — metres, at least 0; read by Volumetric only.
//   wind_direction  — degrees, any finite, the way the wind blows towards:
//                     0 towards -Z, 90 towards +X.
//   wind_speed      — metres a second, at least 0.
//   shadow_strength — 0 to 1, how dark the clouds' shadow is. It is the
//                     clouds' own, not the light's fill: the fill still sets
//                     how dark the shadows of objects are.
//
// THE DRIFT, voe_scene_clouds_drift, IS HOW FAR THE CLOUDS HAVE MOVED: metres
// along x and z, in double, one per clouds row, runtime-only and never saved.
// The clouds system adds it, moves it by the wind and drops it with the row.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_clouds_description(),
// and one that does not has the same struct and nothing more. Every field is
// authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The kind values. A future value is a new constant here, never a reuse of
// one of these.
#define VOE_SCENE_CLOUDS_FLAT 0u
#define VOE_SCENE_CLOUDS_VOLUMETRIC 1u

// The name of each kind value, indexed by the value's own number.
extern VOE_BASE_IMPORTED const char
	*const voe_scene_clouds_kind_names[VOE_SCENE_CLOUDS_VOLUMETRIC + 1];

// Bounds per field — see the header.
#define VOE_SCENE_CLOUDS_FIELDS(F, F_READ_ONLY) \
	F(uint32_t, kind, UINT32)               \
	F(float, coverage, FLOAT32)             \
	F(float, height, FLOAT32)               \
	F(float, thickness, FLOAT32)            \
	F(float, wind_direction, FLOAT32)       \
	F(float, wind_speed, FLOAT32)           \
	F(float, shadow_strength, FLOAT32)

#define VOE_SCENE_CLOUDS_NAMES(N) N(kind, voe_scene_clouds_kind_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(voe_scene_clouds, VOE_SCENE_CLOUDS_FIELDS,
			       VOE_SCENE_CLOUDS_NAMES)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_clouds_key;

// NULL when the entity has no clouds, or is not alive any more. The pointer is
// into the table and is good until the next add or remove.
const voe_scene_clouds *voe_scene_clouds_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

// The table, for the system that hands the clouds to the GPU. rows[i] belongs
// to entities[i], and both are `count` long. The one used is the first row
// whose entity voe_scene_identity_off says is on (0440).
uint32_t voe_scene_clouds_count(const voe_ecs_world *world);
const voe_scene_clouds *voe_scene_clouds_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_clouds_entities(const voe_ecs_world *world);

// Metres the clouds have moved on x and z since the row's drift was added.
typedef struct {
	double x;
	double z;
} voe_scene_clouds_drift;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_clouds_drift_key;

// NULL when the entity has no drift yet, or is not alive.
const voe_scene_clouds_drift *
voe_scene_clouds_drift_get(const voe_ecs_world *world, voe_ecs_entity entity);
