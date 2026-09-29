// What the editor does to which entities exist and which rows they hold: add
// one with Add entity, give one a component or take one away, delete one and
// duplicate one. The Scene panel's Add entity calls voe_editor_entities_add;
// Delete, Duplicate, Add component and Remove call the rest.
//
//     voe_ecs_entity made;
//     if (!voe_editor_entities_add(world, &made))
//             ...                     // "The scene is full."
//
// AN ADD MAKES AN IDENTITY AND A TRANSFORM AND NOTHING ELSE (ADR-0217). There
// is no shortcut that makes an entity with other components on it; everything
// else comes from Add component afterwards. The other makes are a model
// dropped into a view (assets_drag.h): an identity, a transform where it
// landed and the model row naming the file, as 0277 point 8 asks; and a prefab
// dropped the same way: the prefab row instead (0283 point 7), the rest of the
// copy expanded onto it by the world step (prefabs.h).
//
// A NEW COLLIDER STARTS OUT FITTING THE SHAPE (0253). Added to an entity with
// a shape, it is voe_3d_shape_collider's row for that shape's kind, not the
// type's default box, so a capsule gets a capsule without a person typing one.
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
// ITS NAME is the base name — "Entity" for an add, the file's last name less
// `.glb` for a model or `.prefab` for a prefab, and for a duplicate its source's name less a trailing " <number>" — when
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

#include <math/double3.h>

// Makes an entity and queues its two rows: an identity with a new id and the
// name "Entity" by the rules above, and the transform's default row (the
// origin). Writes the entity to `out`. False when the world or the queue is
// full, and then nothing is left of it.
[[nodiscard]] bool voe_editor_entities_add(voe_ecs_world *world,
					   voe_ecs_entity *out);

// As voe_editor_entities_add, named after `path`'s last name less `.glb`, the
// transform at `position` and a model row naming `path` (shorter than
// VOE_3D_MODEL_PATH, project-relative with `/`). False when the world or the
// queue is full, and then nothing is left of it.
[[nodiscard]] bool voe_editor_entities_model_add(voe_ecs_world *world,
						 const char *path,
						 voe_math_double3 position,
						 voe_ecs_entity *out);

// As voe_editor_entities_model_add, named after `path`'s last name less
// `.prefab`, with a prefab row naming `path` (shorter than
// VOE_SCENE_PREFAB_PATH) in place of the model row.
[[nodiscard]] bool voe_editor_entities_prefab_add(voe_ecs_world *world,
						  const char *path,
						  voe_math_double3 position,
						  voe_ecs_entity *out);

// Queues that type's default row onto the entity, or for a collider on an
// entity with a shape the one fitting it. False when the queue is full.
// A type with no default row is the caller's bug and asserts.
[[nodiscard]] bool voe_editor_entities_component_add(voe_ecs_world *world,
						     voe_ecs_entity entity,
						     voe_ecs_type type);

// Queues the removal of the entity's row of that type; the parent's goes
// through voe_scene_parent_set, so the entity keeps its world place. False when
// the queue is full.
[[nodiscard]] bool voe_editor_entities_component_remove(voe_ecs_world *world,
							voe_ecs_entity entity,
							voe_ecs_type type);

// Queues the destruction of the entity and everything under it, its tree
// (scene/parent_component.h). False when the queue is full.
[[nodiscard]] bool voe_editor_entities_delete(voe_ecs_world *world,
					      voe_ecs_entity entity);

// Makes an entity and queues a copy of every described row `source` has, the
// identity's id and name replaced by the rules above. A placed copy's root (a
// prefab row) gives only its identity, transform, parent and prefab rows, the
// four it is saved as (0283), so the world step expands the new copy from its
// file the next frame (prefabs.h). Writes the copy to `out`.
// False when the world or the queue is full, and then nothing is left of it.
[[nodiscard]] bool voe_editor_entities_duplicate(voe_ecs_world *world,
						 voe_ecs_entity source,
						 voe_ecs_entity *out);
