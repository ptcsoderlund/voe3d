// The only way a body changes from outside: an intent, drained by this system.
//
//     voe_physics_body_register(world, 16);             // after colliders
//     voe_physics_body_add(world, player, (voe_physics_body){
//             .step_height = 0.3f, .slope_limit = 0.8f });
//
//     voe_physics_body_submit(world, (voe_physics_body_intent){
//             .entity = player, .body = wanted });      // velocity to move
//
//     voe_physics_body_system_run(world);               // every frame
//
// THE INTENT CARRIES THE WHOLE BODY AND NOT A DELTA, and is registered as the
// component's replace (ecs/component.h), so an inspector finds the door. Two
// submitters in one drain resolve as last-writer-wins.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT: adding a body is what makes it
// exist.
//
// THE DRAIN RUNS EVERY FRAME, in the editor and the game, so an Inspector edit
// lands while nothing plays. The move (card 13) runs only in the game's fixed
// steps and writes velocity and on_floor itself.
//
// THE DRAIN SETTLES EVERY INTENT. A step height below 0 or not finite, a slope
// limit outside [0, π/2] or not finite, or a velocity not finite keeps the last
// row and is an error. The report is physics/collider_system.h's: one line at
// the first settled intent of a run and one closing it, per process.
//
// AN INTENT NAMING AN ENTITY WITH NO BODY IS DROPPED, silently.
//
// IT SITS AT "Physics / Kinematic Body" IN ADD COMPONENT (ecs/component.h).
#pragma once

#include <ecs/world.h>
#include <physics/body_component.h>

#include <stdint.h>

// Registers the table, its description, the intent as its replace, the default
// row (step 0.3, slope 0.8, still, not on the floor) and that it needs a
// collider. Once per world, after colliders are registered. capacity is how
// many bodies the world may hold and how many intents may wait at once.
void voe_physics_body_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its body. False when the table is full or the entity is not
// alive. A missing collider is not refused here; the move finds it.
[[nodiscard]] bool voe_physics_body_add(voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_physics_body body);

// Make this entity's body the one the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_physics_body body;
} voe_physics_body_intent;

// False when the queue is full.
[[nodiscard]] bool voe_physics_body_submit(voe_ecs_world *world,
					   voe_physics_body_intent intent);

// Settles and applies every waiting intent, in submission order, and empties
// the queue.
void voe_physics_body_system_run(voe_ecs_world *world);
