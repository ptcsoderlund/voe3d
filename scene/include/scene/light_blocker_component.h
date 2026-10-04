// A light blocker: a box that keeps light from outside out and light from
// inside in (0347). Read by anyone, const; written only through
// scene/light_blocker_system.h.
//
// WHAT IT SPLITS: a surface inside the box is not reached by the sun or by a
// lamp outside it, and a lamp inside it lights nothing outside — the house
// with a roof, lit by its own lamps, in a sunny world. The rule is the
// renderer's; this row only says where the box is.
//
// THE BOX IS PLACED, TURNED AND SCALED BY ITS TRANSFORM, so a parent carries
// it and the move, rotate and scale gizmos edit it with no case of their own.
// SIZE IS METRES ALONG ITS LOCAL AXES BEFORE SCALE, the whole edge and not a
// half, centred on the transform's position. It is finite and not negative.
//
// IT DRAWS NOTHING, CASTS NOTHING AND COLLIDES WITH NOTHING. It is not a mesh,
// a shadow caster or a collider; an editor shows the selected one's lines.
//
// HOW MANY A WORLD HOLDS IS THE CALLER'S CAPACITY, given at registration.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has
// voe_scene_light_blocker_description(), and one that does not has the same
// struct and nothing more. The one field is authored, not read-only.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

#include <stdint.h>

// Metres along the box's local axes before scale, each finite and not
// negative — see the header.
#define VOE_SCENE_LIGHT_BLOCKER_FIELDS(F, F_READ_ONLY) \
	F(voe_math_float3, size, FLOAT3)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_light_blocker,
			 VOE_SCENE_LIGHT_BLOCKER_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_light_blocker_key;

// NULL when the entity has no blocker, or is not alive any more. The pointer
// is into the table and is good until the next add or remove.
const voe_scene_light_blocker *
voe_scene_light_blocker_get(const voe_ecs_world *world, voe_ecs_entity entity);

// The table, for the system that hands the boxes to the GPU. rows[i] belongs
// to entities[i], and both are `count` long.
uint32_t voe_scene_light_blocker_count(const voe_ecs_world *world);
const voe_scene_light_blocker *
voe_scene_light_blocker_rows(const voe_ecs_world *world);
const voe_ecs_entity *
voe_scene_light_blocker_entities(const voe_ecs_world *world);
