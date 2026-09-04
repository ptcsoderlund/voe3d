// The only way a transform changes: an intent, drained by this system.
//
//     voe_scene_transform_register(world, 4096);            // once, at startup
//     voe_scene_transform_add(world, entity, (voe_scene_transform){ ... });
//
//     // from anywhere, any number of times:
//     voe_scene_transform_submit(world, (voe_scene_transform_intent){
//             .entity = entity, .transform = moved });
//
//     // once a frame, in the frame loop:
//     voe_scene_transform_system_run(world);
//
// THE INTENT CARRIES THE WHOLE TRANSFORM AND NOT A DELTA. A submitter that wants
// to move one thing reads the current transform — reading is anybody's — changes
// the field it cares about and submits the result. That is one intent type
// instead of three, and it makes two submitters in one frame resolve the way
// last-writer-wins rather than by adding up in an order nobody chose.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, which is the one asymmetry here.
// An intent is applied to a component that exists; giving an entity its first
// transform is what makes it exist, and it happens where the entity is being
// built rather than in the middle of a frame. The importer uses this and so does
// any call site placing something by hand.
//
// AN INTENT NAMING A DESTROYED ENTITY IS DROPPED, SILENTLY AND ON PURPOSE. An
// entity dying between a submit and the drain is ordinary — it is what a queue
// costs — and it is not the submitter's mistake.
#pragma once

#include <ecs/world.h>
#include <scene/transform_component.h>

#include <stdint.h>

// Registers the table and the intent queue. Call it once per world, before
// anything adds a transform. capacity is how many transforms the world may hold
// and also how many intents may be waiting at once: one submission per
// transform per frame is what that sizing assumes.
void voe_scene_transform_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its transform. False when the table is full or the entity is
// not alive.
[[nodiscard]] bool voe_scene_transform_add(voe_ecs_world *world,
					   voe_ecs_entity entity,
					   voe_scene_transform transform);

// Put this entity's transform where the submitter says.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_transform transform;
} voe_scene_transform_intent;

// False when the queue is full — the system has not run for long enough, and
// the caller is the one that can do something about that.
[[nodiscard]] bool voe_scene_transform_submit(voe_ecs_world *world,
					      voe_scene_transform_intent intent);

// Applies every waiting intent, in submission order, and empties the queue.
void voe_scene_transform_system_run(voe_ecs_world *world);
