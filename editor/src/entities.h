// What the editor does to which entities exist and which rows they hold: add
// one from the Add menu, give one a component or take one away, delete one and
// duplicate one. The Scene panel's Add calls voe_editor_entities_add; Delete,
// Duplicate, Add component and Remove call the rest.
//
//     voe_ecs_entity made;
//     if (!voe_editor_entities_add(world, VOE_EDITOR_ADD_CUBE, &made))
//             ...                     // "The scene is full."
//
// EVERYTHING GOES THROUGH THE STRUCTURAL QUEUE (ADR-0193, 0190). The entity is
// made here with voe_ecs_entity_create, and every row it is given is a
// voe_ecs_structure_add — never a folder's typed creation call. Nothing appears
// in a table until the program applies the queue, once a frame before the
// systems run (main.c). Nothing in here draws, selects or marks a project
// unsaved; the caller does each of those.
//
// A NEW ENTITY'S ID is one more than the largest identity id in the world, which
// cannot collide with any id in the file (ADR-0193).
//
// ITS NAME is the base name — "Entity", "Cube", "Capsule" or "Cylinder" for an
// add, and for a duplicate its source's name less a trailing " <number>" — when
// no identity has that name, and otherwise "<base> N" for the lowest N from 2
// that no identity has. A base too long to take the suffix in
// VOE_SCENE_IDENTITY_NAME bytes is cut short to make room for it.
//
// A FAILED ADD OR DUPLICATE LEAVES NOTHING BEHIND. The entity is destroyed
// through the queue, after whatever adds were queued for it, so they apply to a
// stale id and are dropped; when the queue has no room even for that, it is
// destroyed at once — it holds no row yet, all of them still waiting in the
// queue, so that changes no row that exists, and the queued adds drop as stale
// all the same.
//
// CONSTRAINTS. The ids and names are read from the identity table as it stands,
// so two adds in one frame, before the queue is applied, would be given the
// same id and name; one per frame is what the menu and a keyboard command can
// ask for. Lifting that means counting the identities waiting in the queue too.
// Finding a name is a scan of the identity table per candidate, which the
// table's VOE_EDITOR_SCENE_ROWS rows keep cheap.
#pragma once

#include <ecs/component.h>
#include <ecs/world.h>

// What the Add menu makes: an entity with only an identity, or one of the three
// shapes with an identity, a transform and a shape.
typedef enum {
	VOE_EDITOR_ADD_ENTITY,
	VOE_EDITOR_ADD_CUBE,
	VOE_EDITOR_ADD_CAPSULE,
	VOE_EDITOR_ADD_CYLINDER
} voe_editor_add;

// Makes an entity and queues its rows at their defaults: an identity with a new
// id and name, and for a shape the transform's default row and the shape's with
// `kind` set. Writes the entity to `out`. False when the world or the queue is
// full, and then nothing is left of it.
[[nodiscard]] bool voe_editor_entities_add(voe_ecs_world *world,
					   voe_editor_add what,
					   voe_ecs_entity *out);

// Queues that type's default row onto the entity. False when the queue is full.
// A type with no default row is the caller's bug and asserts.
[[nodiscard]] bool voe_editor_entities_component_add(voe_ecs_world *world,
						     voe_ecs_entity entity,
						     voe_ecs_type type);

// Queues the removal of the entity's row of that type. False when the queue is
// full.
[[nodiscard]] bool voe_editor_entities_component_remove(voe_ecs_world *world,
							voe_ecs_entity entity,
							voe_ecs_type type);

// Queues the entity's destruction. False when the queue is full.
[[nodiscard]] bool voe_editor_entities_delete(voe_ecs_world *world,
					      voe_ecs_entity entity);

// Makes an entity and queues a copy of every described row `source` has, the
// identity's id and name replaced by the rules above. Writes the copy to `out`.
// False when the world or the queue is full, and then nothing is left of it.
[[nodiscard]] bool voe_editor_entities_duplicate(voe_ecs_world *world,
						 voe_ecs_entity source,
						 voe_ecs_entity *out);
