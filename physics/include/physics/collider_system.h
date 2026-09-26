// The only way a collider changes: an intent, drained by this system.
//
//     voe_physics_collider_register(world, 256);        // after transforms
//     voe_physics_collider_add(world, wall, (voe_physics_collider){
//             .kind = VOE_PHYSICS_COLLIDER_BOX, .size = { 1, 1, 1 } });
//
//     voe_physics_collider_submit(world, (voe_physics_collider_intent){
//             .entity = wall, .collider = changed });
//
//     voe_physics_collider_system_run(world);           // once a step
//
// THE INTENT CARRIES THE WHOLE COLLIDER AND NOT A DELTA, and is registered as
// the component's replace (ecs/component.h), so an inspector that knows nothing
// about this folder finds the door. Two submitters in one drain resolve as
// last-writer-wins.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT: an intent changes a collider that
// exists, and adding one is what makes it exist.
//
// THE DRAIN SETTLES EVERY INTENT, WHICHEVER DOOR IT CAME THROUGH. A size with a
// component that is not finite or is below 0 keeps the last row and is an
// error; an unknown kind is applied and warned about, because a file may be
// newer than the build (physics/collider_component.h).
//
// THE REPORT IS scene/transform_system.h's: one line naming the first settled
// intent of a run, and one at the first drain that settles none, with the count,
// reading `error` when anything was kept. The state is per process, for the
// reason that header gives. The entity is named by index and generation.
//
// AN INTENT NAMING AN ENTITY WITH NO COLLIDER IS DROPPED, silently: an entity
// dying between a submit and the drain is ordinary.
//
// IT SITS AT "Physics / Collider" IN ADD COMPONENT (ecs/component.h, 0221).
#pragma once

#include <ecs/world.h>
#include <physics/collider_component.h>

#include <stdint.h>

// Registers the table, its description, the intent as its replace, the default
// row (a box of 1, not a trigger) and that it needs a transform. Once per world,
// after transforms are registered. capacity is how many colliders the world may
// hold and how many intents may wait at once.
void voe_physics_collider_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its collider. False when the table is full or the entity is
// not alive. A missing transform is not refused here; physics/shape.h finds it.
[[nodiscard]] bool voe_physics_collider_add(voe_ecs_world *world,
					    voe_ecs_entity entity,
					    voe_physics_collider collider);

// Make this entity's collider the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_physics_collider collider;
} voe_physics_collider_intent;

// False when the queue is full.
[[nodiscard]] bool voe_physics_collider_submit(
	voe_ecs_world *world, voe_physics_collider_intent intent);

// Settles and applies every waiting intent, in submission order, and empties
// the queue.
void voe_physics_collider_system_run(voe_ecs_world *world);
