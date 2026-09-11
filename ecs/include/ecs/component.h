// Component tables. One contiguous array per component type, plus the two
// directions between an entity and its row.
//
//     const struct voe_ecs_key my_key = { "my_component" };
//
//     voe_ecs_type type = voe_ecs_component_register(world, &my_key,
//                                                    sizeof(struct mine), 1024,
//                                                    NULL);
//     if (!voe_ecs_component_add(world, type, entity, &value))
//             ...                                  // the table is full
//
//     const struct mine *rows = voe_ecs_component_rows(world, type);
//     const voe_ecs_entity *owners = voe_ecs_component_entities(world, type);
//     for (uint32_t i = 0; i < voe_ecs_component_count(world, type); i++)
//             ...                                  // rows[i] belongs to owners[i]
//
// ITERATING IS A LINEAR WALK OVER ONE ARRAY AND THAT PROPERTY IS THE POINT. The
// rows are packed from 0 to count with no holes and no pointers in between, so a
// system that reads every one of them reads memory in order. _rows and
// _entities hand out the two arrays themselves rather than a per-row accessor,
// because an accessor per row is the pointer chasing this shape exists to avoid.
//
// ENTITY TO ROW IS A DIRECT INDEX, PAID FOR IN MEMORY. Each table keeps one
// uint32_t per entity slot the world can hold, so a lookup by entity is one
// load and a bounds check, and a table of a rare component costs the world's
// entity count in indices whether or not anything has one. That is the trade
// this engine took: tables are few and entity counts are bounded, so four bytes
// per slot per type is cheaper to reason about than a hash. A sparse set would
// swap that memory for a second indirection, and nothing has needed to yet.
//
// REMOVING SWAPS THE LAST ROW INTO THE HOLE, so row order is not insertion order
// and does not survive a removal. Anything that depends on the order rows come
// out in is depending on a thing this table does not promise; what it does
// promise is that every live row is visited exactly once.
//
// A COMPONENT IS READ BY ANYONE AND WRITTEN BY ONE. Everything here that reads
// is const, and everything that writes is meant to be called from inside the
// module that owns the type — which is why the typed wrapper a folder exposes
// (voe_scene_transform_get and friends) is the surface everyone else uses.
// Nothing in ecs can enforce that; the folder that owns the component is what
// enforces it, by exposing reads publicly and keeping the writes to itself.
//
// A STALE ENTITY IS REFUSED RATHER THAN ANSWERED. Every function here checks the
// generation, so an id from a destroyed entity gets NULL or false and never the
// row of whatever now lives in that slot.
//
// THE WORLD LISTS ITS TYPES, AND WHAT AN ENTITY IS MADE OF IS THE CALLER'S LOOP.
// Types are enumerated in registration order, and an entity's makeup is found by
// asking every one of them for a row:
//
//     for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
//             voe_ecs_type type = voe_ecs_component_type_at(world, i);
//             if (voe_ecs_component_get(world, type, entity) != NULL)
//                     ...          // voe_ecs_component_key and _description
//     }
//
// There is no per-entity set of types to keep in step: the direct index above is
// already the answer, one load per type.
//
// A DESCRIPTION IS STORED AND NEVER READ. A registration may carry the struct
// description its folder wrote (base/describe.h), and the world hands the same
// pointer back — it does not know what a field is, and a NULL there means only
// that nobody described that component. The pointer must outlive the world, which
// a description's static table always does.
#pragma once

#include <base/describe.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

// A registered component type. Passed by value; means nothing outside the world
// that handed it out.
typedef struct {
	uint32_t value;
} voe_ecs_type;

// Registering twice with the same key, registering more types than the world was
// made for, a size of zero or a capacity of zero are all the caller's bugs and
// assert. Nothing about a registration comes out of a file. `description` may be
// NULL, and is for an undescribed component.
voe_ecs_type voe_ecs_component_register(
	voe_ecs_world *world, const struct voe_ecs_key *key, size_t size,
	uint32_t capacity, const voe_base_struct_description *description);

// How many types have been registered. The types are 0 to count, in the order
// they were registered.
uint32_t voe_ecs_component_type_count(const voe_ecs_world *world);

// The type registered `index`-th. An index at or past the count asserts.
voe_ecs_type voe_ecs_component_type_at(const voe_ecs_world *world,
				       uint32_t index);

// The key the type was registered against — its name is what a caller shows.
const struct voe_ecs_key *voe_ecs_component_key(const voe_ecs_world *world,
						voe_ecs_type type);

// The description the type was registered with, or NULL when it had none.
const voe_base_struct_description *
voe_ecs_component_description(const voe_ecs_world *world, voe_ecs_type type);

// The type registered against that key. Asserts if nothing was — a module asking
// for its own type before it registered it is a bug in the order the program
// starts up, not a thing to handle.
voe_ecs_type voe_ecs_component_type(const voe_ecs_world *world,
				    const struct voe_ecs_key *key);

// Copies `value` in as the entity's row. False when the table is full or the
// entity is not alive; a caller cannot tell those apart and does not need to,
// because neither is something to retry. Giving an entity a second component of
// a type it already has is the caller's bug and asserts.
[[nodiscard]] bool voe_ecs_component_add(voe_ecs_world *world, voe_ecs_type type,
					 voe_ecs_entity entity,
					 const void *value);

// Overwrites the row the entity already has. False when it has none — which is
// the ordinary answer for an intent that named an entity that has since been
// destroyed, and the reason this returns rather than asserts.
[[nodiscard]] bool voe_ecs_component_set(voe_ecs_world *world, voe_ecs_type type,
					 voe_ecs_entity entity,
					 const void *value);

// NULL when the entity has no row of this type, is not alive, or never was. The
// pointer is into the table, so it is valid until the next add or remove of this
// type and no longer.
const void *voe_ecs_component_get(const voe_ecs_world *world, voe_ecs_type type,
				  voe_ecs_entity entity);

// True when there was one to remove.
bool voe_ecs_component_remove(voe_ecs_world *world, voe_ecs_type type,
			      voe_ecs_entity entity);

uint32_t voe_ecs_component_count(const voe_ecs_world *world, voe_ecs_type type);

// The rows, packed, `count` of them. Cast it to the component's own type; the
// module that registered the type is the one that knows what it is.
const void *voe_ecs_component_rows(const voe_ecs_world *world,
				   voe_ecs_type type);

// Who each row belongs to, same order, same count. This is how a system that has
// walked one table looks the second component up.
const voe_ecs_entity *voe_ecs_component_entities(const voe_ecs_world *world,
						 voe_ecs_type type);
