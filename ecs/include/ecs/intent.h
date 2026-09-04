// Intent queues. A queue per intent type: submitting is copying a value into it,
// and nothing happens until the system that owns the type drains it.
//
//     const struct voe_ecs_key my_key = { "my_intent" };
//     voe_ecs_intent type = voe_ecs_intent_register(world, &my_key,
//                                                   sizeof(struct mine), 64);
//
//     if (!voe_ecs_intent_submit(world, type, &value))
//             ...                                    // the queue is full
//
//     // ... in the owning system, and nowhere else:
//     const struct mine *queued = voe_ecs_intent_queue(world, type);
//     for (uint32_t i = 0; i < voe_ecs_intent_count(world, type); i++)
//             apply(&queued[i]);
//     voe_ecs_intent_clear(world, type);
//
// THIS IS THE ONLY WAY ONE MODULE CHANGES ANOTHER MODULE'S DATA (rule 4). A
// system never calls another system; it submits a value and the owner applies
// it. That is what keeps the dependency graph a graph of data rather than of
// calls, and it is the seam a later card needs in order to let several threads
// submit while one drains.
//
// SUBMIT FROM ANYWHERE, DRAIN IN ONE PLACE. Every submitter's value lands in the
// same queue in submission order, and the owning system sees them in that order.
// Two submitters do not interleave with each other in any other way, and there
// is no priority.
//
// WHEN AN INTENT TAKES EFFECT IS "THE NEXT TIME ITS SYSTEM RUNS", AND THAT IS
// ALL THIS PROMISES. Depending on the order the frame runs its systems in, that
// may be the same frame or the next one. Nothing here promises the same frame,
// and code that needs an answer back this instant wants a function call to the
// owner, which is exactly what rule 4 forbids — so it wants a different design.
//
// A DRAIN IS THE OWNER'S TO DO. Reading the queue does not empty it; _clear does.
// They are separate so that the owner reads what it can apply and empties once
// afterwards, and so that a queue that was never drained is visible as a count
// that keeps growing rather than as work that silently vanished.
#pragma once

#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

// A registered intent type. Deliberately not the same type as voe_ecs_type: a
// component table and an intent queue are different things and mixing the two
// ids up should not compile.
typedef struct {
	uint32_t value;
} voe_ecs_intent;

// The same rules as a component registration: a repeated key, more types than
// the world was made for, a zero size or a zero capacity are bugs and assert.
voe_ecs_intent voe_ecs_intent_register(voe_ecs_world *world,
				       const struct voe_ecs_key *key,
				       size_t size, uint32_t capacity);

// Asserts if nothing was registered against that key.
voe_ecs_intent voe_ecs_intent_type(const voe_ecs_world *world,
				   const struct voe_ecs_key *key);

// Copies the value onto the end of the queue. False when the queue is full,
// which is the one way it fails — a system that has not run for long enough to
// drain, or a submitter in a loop, and either way the caller is the one that can
// do something about it.
[[nodiscard]] bool voe_ecs_intent_submit(voe_ecs_world *world,
					 voe_ecs_intent type,
					 const void *value);

uint32_t voe_ecs_intent_count(const voe_ecs_world *world, voe_ecs_intent type);

// The queued values, in submission order, `count` of them. Cast to the intent's
// own type.
const void *voe_ecs_intent_queue(const voe_ecs_world *world,
				 voe_ecs_intent type);

// Empties it. The owning system's last act.
void voe_ecs_intent_clear(voe_ecs_world *world, voe_ecs_intent type);
