// What an ENTITY field offers and what it says: the choices its dropdown lists
// and the one word each is shown by. inspector.c draws the list from the
// choices and labels the field's button, and inspector_value.c labels an
// ENTITY that is only shown.
//
//     uint32_t count = voe_editor_entity_field_choices(world, out, room);
//     const char *shown = voe_editor_entity_field_label(world, entity, arena);
//
// A PERSON PICKS BY NAME AND NEVER BY ID (ADR-0242 point 9). An entity's
// index and generation are the world's bookkeeping, change on every load and
// mean nothing to whoever authored "Player"; the identity's name is what the
// Scene panel shows the same entity as, so the field and the list agree.
//
// ONLY AUTHORED ENTITIES ARE OFFERED, because only they are saved: an entity
// with no identity is one the engine made for itself (ADR-0125), and a field
// pointing at it would point at nothing once the scene is written and read
// back. The first choice is None, a zeroed entity, which no live entity ever
// is (ecs/world.h); the rest go ascending by identity id, the order they were
// authored in and the one that does not move when a name changes.
//
// Constraints: the identity table holds VOE_GAME_WORLD_AUTHORED rows
// (game/world.h), so the sort is an insertion sort over at most that many and
// the list is never longer than one more; raising that number lifts both.
#pragma once

#include <base/arena.h>

#include <ecs/world.h>

#include <stdint.h>

// Fills `out`, which has `room` entries and at least one, with None and then
// every authored entity ascending by identity id, and returns how many it
// wrote. Choices past `room` are left out.
uint32_t voe_editor_entity_field_choices(const voe_ecs_world *world,
					 voe_ecs_entity *out, uint32_t room);

// The identity's name of `entity`, copied into `arena`, or "None" for a
// zeroed entity, a dead one or one with no identity.
const char *voe_editor_entity_field_label(const voe_ecs_world *world,
					  voe_ecs_entity entity,
					  voe_base_arena *arena);
