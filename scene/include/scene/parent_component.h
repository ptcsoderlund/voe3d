// Which thing a thing hangs under: one entity, its parent, and the walks up and
// down the tree that rows of it make. Read by anyone, const; written only
// through scene/parent_system.h.
//
// AN ENTITY WITH NO ROW IS A ROOT (0281 point 1). Its transform row is then its
// world place, as every transform was before parenting existed.
//
// AN ENTITY WITHOUT A TRANSFORM MAY BE A PARENT OR A CHILD (0300), so a bare
// entity such as "Lamps" groups a hundred lights under it.
//
// IT IS A ROW OF ITS OWN AND NOT A FIELD ON THE TRANSFORM, so every scene saved
// before parenting reads with no warning, and scene text, undo and the cook
// carry the ENTITY field as an authored id with nothing new.
//
// IT HAS NO REPLACE INTENT AND NO MENU PATH, so Add component never offers it
// and the Inspector shows it without editing it. Parenting is the call
// parent_system.h holds, which keeps the child's world place; editing the field
// would not.
//
// EVERY WALK STOPS AT VOE_SCENE_PARENT_DEPTH_MAX LINKS (0281 point 6). A loop can
// only come from a hand-edited file, and it ends the walk there instead of
// hanging. A parent that is dead, or has no transform, ends the chain as if
// there were none.
//
// THE STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a build
// that asks for descriptions also has voe_scene_parent_description().
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

// The entity this one hangs under; zeroed is none.
#define VOE_SCENE_PARENT_FIELDS(F, F_READ_ONLY) \
	F(voe_ecs_entity, parent, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_parent, VOE_SCENE_PARENT_FIELDS)

// How many links any walk follows before it stops.
#define VOE_SCENE_PARENT_DEPTH_MAX 32

// The key this component is registered against. Its address is its identity.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_parent_key;

// NULL when the world never registered the table, or the entity has no row. The
// pointer is into the table and is good until the next add or remove.
const voe_scene_parent *voe_scene_parent_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);

// True when `ancestor` is `entity` or anywhere above it.
bool voe_scene_parent_within(const voe_ecs_world *world, voe_ecs_entity entity,
			     voe_ecs_entity ancestor);

// Writes `root`, then everything under it depth-first, children in the parent
// table's row order. Returns how many it wrote, stopping at `capacity`.
uint32_t voe_scene_parent_tree(const voe_ecs_world *world, voe_ecs_entity root,
			       voe_ecs_entity *out, uint32_t capacity);
