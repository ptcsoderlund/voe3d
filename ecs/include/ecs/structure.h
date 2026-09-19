// The world's structural queue: which rows exist changes here and nowhere else
// (0190). Three requests — add a row with given bytes, remove a row, destroy an
// entity — queued as they are submitted and applied together at one point in
// the frame.
//
//     voe_ecs_world *world = voe_ecs_world_new(arena, (voe_ecs_limits){
//             .entities = 4096, .component_types = 8, .intent_types = 8,
//             .structure_requests = 256, .structure_bytes = 16384 });
//
//     if (!voe_ecs_structure_add(world, type, entity,
//                                voe_ecs_component_default(world, type)))
//             ...                          // the queue is full this frame
//
//     // once a frame, in the program, before any system runs:
//     voe_ecs_structure_apply(world);
//
// ANYONE MAY SUBMIT. The editor, undo and game logic all change which rows exist,
// and none of them owns the rows they add. What a row holds stays its system's
// (rule 4); the queue decides only whether the row is there.
//
// THE PROGRAM CALLS _apply ONCE A FRAME, BEFORE ITS SYSTEMS RUN, so no system sees
// a row appear or vanish partway through its run. A system walking a table can
// submit a remove for the row it is looking at and keep walking.
//
// REQUESTS APPLY IN SUBMISSION ORDER. An add then a remove of the same row leaves
// nothing; a destroy then an add to that entity leaves nothing, because by then
// the id is stale.
//
// A REQUEST THAT NO LONGER MAKES SENSE IS DROPPED, SILENTLY. An add whose entity
// is dead, already has that type, or whose table is full; a remove with no row;
// a destroy of a stale id. Between submit and apply anything may have happened
// to the entity, so a request naming one that is gone is the queue's ordinary
// cost, as an intent naming a dead entity is — not a bug, and nothing anyone
// could act on by then. Nothing is reported.
//
// A FULL QUEUE IS THE ONE RETURNED FAILURE. Its room is two numbers fixed at
// creation (voe_ecs_limits): how many requests, and how many bytes of rows the
// adds carry. A world made with either at zero has no queue and refuses every
// submit.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdint.h>

// Queues an add of `row` — the type's registered size, copied now — to
// `entity`. False only when the queue has no room for the request or its bytes.
[[nodiscard]] bool voe_ecs_structure_add(voe_ecs_world *world,
					 voe_ecs_type type,
					 voe_ecs_entity entity,
					 const void *row);

// Queues a removal of the entity's row of `type`. False only when the queue is
// full.
[[nodiscard]] bool voe_ecs_structure_remove(voe_ecs_world *world,
					    voe_ecs_type type,
					    voe_ecs_entity entity);

// Queues the entity's destruction, which takes every row it has. False only when
// the queue is full.
[[nodiscard]] bool voe_ecs_structure_destroy(voe_ecs_world *world,
					     voe_ecs_entity entity);

// Applies every waiting request in submission order, dropping the ones that no
// longer make sense, then empties the queue.
void voe_ecs_structure_apply(voe_ecs_world *world);

// How many requests are waiting.
uint32_t voe_ecs_structure_count(const voe_ecs_world *world);
