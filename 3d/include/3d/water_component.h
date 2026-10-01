// A body of water an entity carries, its replace intent, and the clock its
// waves run on. Read by anyone, const; written through the submit here and the
// water system (voe_3d_water_system_run), which is the one place rows change.
//
//     voe_scene_transform_register(world, capacity);   // first: water needs one
//     voe_3d_water_register(world, capacity);
//     if (!voe_3d_water_submit(world, (voe_3d_water_intent){
//                 .entity = entity, .water = water }))
//             ...                                       // the queue is full
//
// WATER IS A FLAT PLANE (0305 point 5): `width` and `length` are its size in
// metres along the transform's X and Z, centred on it; `wave_height` and
// `wave_length` are the waves' amplitude and longest wavelength in metres;
// `deep` is the thickness in metres at which the water is nearly opaque, clear
// at the shore. `colour` is the body's and `sky` the colour fresnel reflects
// toward, both linear rgb. It sits at "Rendering / Water" and needs a transform.
//
// THE DEFAULT ROW is 20 by 20 m, waves 0.05 m high and 2 m long, 3 m deep, a deep
// green-blue body under a pale blue sky.
//
// WAVES ARE DRAWN, NEVER MOVED: they bend the shading normal only, so the plane
// stays where its transform puts it, for the shore's depth fade and for a pick.
//
// THE CLOCK IS A RUNTIME-ONLY ROW, `voe_3d_waves`, one per water, never saved:
// seconds kept below 60, where every wave's phase wraps. The water system owns
// it: adds it, steps it and drops it with the water. The replace intent is the
// whole row, as the emitter's is (3d/emitter_component.h).
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

#define VOE_3D_WATER_FIELDS(F, F_READ_ONLY)     \
	F(float, width, FLOAT32)                \
	F(float, length, FLOAT32)               \
	F(float, wave_height, FLOAT32)          \
	F(float, wave_length, FLOAT32)          \
	F(float, deep, FLOAT32)                 \
	F(voe_math_float3, colour, COLOUR)      \
	F(voe_math_float3, sky, COLOUR)

VOE_BASE_DESCRIBE_STRUCT(voe_3d_water, VOE_3D_WATER_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_water_key;

// Change this entity's water to the submitter's row.
typedef struct {
	voe_ecs_entity entity;
	voe_3d_water water;
} voe_3d_water_intent;

// A water's clock, in seconds, below 60.
typedef struct {
	double seconds;
} voe_3d_waves;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_waves_key;

// Registers the water (its default row, its need of a transform, its menu path
// and its intent as its replace) and the waves as runtime-only, all with room
// for `capacity`. Once per world, after transforms.
void voe_3d_water_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its water. False when the table is full or the entity is not
// alive. A direct call, as the emitter's: an intent changes one that exists.
[[nodiscard]] bool voe_3d_water_add(voe_ecs_world *world, voe_ecs_entity entity,
				    voe_3d_water water);

// NULL when the entity has no water or is not alive.
const voe_3d_water *voe_3d_water_get(const voe_ecs_world *world,
				     voe_ecs_entity entity);

// The table. rows[i] belongs to entities[i], and both are `count` long.
uint32_t voe_3d_water_count(const voe_ecs_world *world);
const voe_3d_water *voe_3d_water_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_water_entities(const voe_ecs_world *world);

// NULL when the entity has no waves row or is not alive.
const voe_3d_waves *voe_3d_waves_get(const voe_ecs_world *world,
				     voe_ecs_entity entity);

// The waves table, as the water's.
uint32_t voe_3d_waves_count(const voe_ecs_world *world);
const voe_3d_waves *voe_3d_waves_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_waves_entities(const voe_ecs_world *world);

// False when the queue is full: the system has not run for long enough.
[[nodiscard]] bool voe_3d_water_submit(voe_ecs_world *world,
				       voe_3d_water_intent intent);
