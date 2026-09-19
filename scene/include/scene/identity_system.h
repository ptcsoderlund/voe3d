// The only way an identity changes: an intent, drained by this system.
//
//     voe_scene_identity_register(world, 4096);             // once, at startup
//     voe_scene_identity_add(world, entity,
//                            (voe_scene_identity){ .id = 1, .name = "Cube" });
//
//     // from anywhere, any number of times:
//     voe_scene_identity_submit(world, (voe_scene_identity_intent){
//             .entity = entity, .identity = renamed });
//
//     // once a frame, in the frame loop:
//     voe_scene_identity_system_run(world);
//
// THE INTENT CARRIES THE WHOLE ROW AND NOT A DELTA, as the transform's does. A
// submitter reads the current identity — reading is anybody's — changes the name
// and submits the result, so two submitters in one frame resolve last-writer-wins
// rather than by combining in an order nobody chose. It is registered as the
// component's replace intent (ecs/component.h), which is how an inspector that
// knows nothing about this folder finds the door to knock on.
//
// CREATION IS A DIRECT CALL AND NOT AN INTENT, the same asymmetry the transform
// has and for the same reason: an intent is applied to a component that exists,
// and giving an entity its identity is what makes it authored. That happens
// where the entity is being built — a loader, or a call site placing something
// by hand — and not in the middle of a frame.
//
// AN INTENT NAMING A DESTROYED ENTITY, OR ONE WITH NO IDENTITY, IS DROPPED,
// SILENTLY AND ON PURPOSE. An entity dying between a submit and the drain is
// ordinary — it is what a queue costs — and it is not the submitter's mistake.
//
// A NAME WITH NO TERMINATING ZERO IS THE PROGRAM'S BUG AT THESE TWO CALLS AND A
// CORRECTION IN THE DRAIN. Both typed calls assert it, because a caller that
// reached one of them had a name in its hand and got it wrong. The drain corrects
// instead, because what reaches it may have come through voe_ecs_intent_submit
// from a tool that only knows the offsets — see scene/identity_component.h for
// what it settles and why each is a correction rather than a refusal.
//
// THE REPORT IS ONE LINE AT THE START OF A RUN AND ONE AT ITS END, AND ITS STATE
// IS PER PROCESS AND NOT PER WORLD. A drain that corrects an intent after a drain
// that corrected none writes one line naming that first intent; every correction
// after it is counted, and the first drain that corrects none closes the run with
// the count. The flag and the counter are file-scope statics in
// identity_system.c, so two worlds in one process share them. That is enough for
// a report: it is there to be noticed by whoever is watching stderr, not to be
// attributed or parsed, and a second world in one process is a test harness
// rather than a shipped program.
#pragma once

#include <ecs/world.h>
#include <scene/identity_component.h>

#include <stdint.h>

// Registers the table, its description, the intent queue, the intent as the
// component's replace, and the default row: id 0 and an empty name. The default
// row is what "add at default" gives (0190). Call it once per world, before
// anything adds an identity.
// capacity is how many identities the world may hold and also how many intents
// may be waiting at once: one submission per identity per frame is what that
// sizing assumes.
void voe_scene_identity_register(voe_ecs_world *world, uint32_t capacity);

// Gives the entity its identity, and with it the mark of having been authored.
// False when the table is full or the entity is not alive. A `name` with no
// terminating zero in its 64 bytes is the caller's bug and asserts.
[[nodiscard]] bool voe_scene_identity_add(voe_ecs_world *world,
					  voe_ecs_entity entity,
					  voe_scene_identity identity);

// Put this entity's identity where the submitter says. The id is read-only: one
// carrying an id other than the entity's own has it put back in the drain, and
// the name beside it still lands.
typedef struct {
	voe_ecs_entity entity;
	voe_scene_identity identity;
} voe_scene_identity_intent;

// False when the queue is full — the system has not run for long enough, and the
// caller is the one that can do something about that. A `name` with no
// terminating zero in its 64 bytes is the caller's bug and asserts.
[[nodiscard]] bool voe_scene_identity_submit(voe_ecs_world *world,
					     voe_scene_identity_intent intent);

// Settles every waiting intent, applies it in submission order, and empties the
// queue.
void voe_scene_identity_system_run(voe_ecs_world *world);
