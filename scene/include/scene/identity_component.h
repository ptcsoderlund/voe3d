// Who a thing is: a 64-bit id, a name and whether its Scene list row is folded,
// on the entities a person authored.
// Read by anyone, const; written only through scene/identity_system.h.
//
// THE COMPONENT IS OPTIONAL AND ITS PRESENCE IS THE MEANING. An entity with an
// identity was authored — it came out of a scene a person made, and it is the
// kind of thing an editor lists and an inspector shows. An entity without one is
// the engine's own: a probe, a gizmo, a thing built for a frame. That is the
// whole of what the component says, and it is why there is no `authored` flag
// anywhere else.
//
// THE ID IS UNIQUE WITHIN ITS FILE AND NOTHING WIDER. It is set when the entity
// is created and never changed after, which is what lets a file refer to an
// entity across a save and a load. Two worlds, or one world holding two loaded
// files, may hold the same id twice and that is not a fault — so nothing here
// allocates an id, checks one against another entity's, or hands out a next one.
//
// THERE IS NO LOOKUP BY ID AND NONE BY NAME. Either would be an index to keep in
// step with a table that already answers the question a walk away, and nothing
// has needed one. A caller wanting the entity with a given id walks _rows and
// _entities, which is the same linear walk everything else here does.
//
// THE ID IS READ-ONLY AND THE NAME IS NOT. The field list marks the id through
// F_READ_ONLY (base/describe.h), so an inspector shows it and offers no way to
// change it; the drain enforces the same thing for a replace that arrives by
// another route. A rename is ordinary and goes through the intent.
//
// FOLDED SAYS THE SCENE LIST SHOWS THIS ENTITY'S CHILDREN FOLDED AWAY (0300), and
// nothing else about the entity changes with it. It lives here so it is saved,
// undone and copied with the identity, with no table for one bit (0302). It goes
// through the replace intent like the name, and the drain corrects nothing about
// it. A file without it reads as false: unfolded.
//
// WHAT THE DRAIN SETTLES, AND WHY EACH IS A CORRECTION RATHER THAN A REFUSAL
// (ADR-0138 — correct where one nearest valid value exists):
//
//   - A NAME WITH NO TERMINATING ZERO has its last byte set to zero. The nearest
//     valid name is the 63 bytes that were readable, and dropping the whole
//     intent would lose an edit the submitter meant.
//   - AN ID DIFFERENT FROM THE ENTITY'S CURRENT ID is put back to the current
//     one, and the rest of the row lands as submitted. There is exactly one
//     valid id for this entity — the one it already has — so the correction is
//     not a guess, and the name beside it is still an edit worth keeping.
//
// Both are reported; scene/identity_system.h says how often.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_identity_description(), and one
// that does not has the same struct and nothing more.
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// The name, terminating zero included, so 63 bytes of text at most. Fixed and
// in the row because a component is plain data with no pointers in it — that is
// what lets a table be walked, copied and one day read from another thread.
#define VOE_SCENE_IDENTITY_NAME 64

#define VOE_SCENE_IDENTITY_FIELDS(F, F_READ_ONLY)         \
	F_READ_ONLY(uint64_t, id, UINT64)                 \
	F(char, name, CHAR, VOE_SCENE_IDENTITY_NAME)      \
	F(bool, folded, BOOL)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_identity, VOE_SCENE_IDENTITY_FIELDS)

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_identity_key;

// NULL when the entity has no identity — which is the ordinary answer for one
// the engine made rather than a person — or is not alive any more. The pointer
// is into the table and is good until the next add or remove.
const voe_scene_identity *voe_scene_identity_get(const voe_ecs_world *world,
						 voe_ecs_entity entity);

// The table, for a system that reads every one of them. rows[i] belongs to
// entities[i], and both are `count` long.
uint32_t voe_scene_identity_count(const voe_ecs_world *world);
const voe_scene_identity *voe_scene_identity_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_scene_identity_entities(const voe_ecs_world *world);
