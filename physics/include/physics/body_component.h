// A kinematic body: how high a step it climbs, how steep a slope it stands on,
// its velocity and whether it is on the floor (0249 layer 2, 0253 point 4).
// Read by anyone, const; written only through physics/body_system.h.
//
// `velocity` GOES BOTH WAYS. When a project submits a row it is the velocity the
// body wants, in metres a second; after a move it is what the body really
// moved, so a project reads a landing or a ceiling from it (a fall that stopped
// is a y that went to 0). `on_floor` is written by the move alone.
//
// GRAVITY AND JUMPING ARE THE PROJECT'S (0249): it adds them to the velocity it
// submits; the body only moves what it is given.
//
// READ-ONLY IS THE INSPECTOR'S NOTE, not a lock: a project still writes the
// whole row, velocity included, through the intent.
//
// `step_height` IS IN METRES and never below 0; `slope_limit` is radians from
// level, in [0, π/2]. The default row is a step of 0.3, a slope of 0.8, still
// and not on the floor. A body needs a collider, so colliders are registered
// first; the move itself is card 13's.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

#define VOE_PHYSICS_BODY_FIELDS(F, F_READ_ONLY)            \
	F(float, step_height, FLOAT32)                     \
	F(float, slope_limit, FLOAT32)                     \
	F_READ_ONLY(voe_math_float3, velocity, FLOAT3)     \
	F_READ_ONLY(bool, on_floor, BOOL)

VOE_BASE_DESCRIBE_STRUCT(voe_physics_body, VOE_PHYSICS_BODY_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_physics_body_key;

// NULL when the entity has no body or is not alive. The pointer is into the
// table and is good until the next add or remove.
const voe_physics_body *voe_physics_body_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

// The table, for the move. rows[i] belongs to entities[i], both `count` long.
uint32_t voe_physics_body_count(const voe_ecs_world *world);
const voe_physics_body *voe_physics_body_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_physics_body_entities(const voe_ecs_world *world);
