// Which prefab a placed copy is, and which placed copy a thing was made for:
// two rows, read by anyone, const; registered by scene/prefab_system.h.
//
// THE PATH IS PROJECT-RELATIVE WITH `/`, as a model's is: `Assets/tank.prefab`,
// never a drive or a backslash, so a scene reads the same on every machine.
//
// THE PREFAB ROW MARKS A PLACED COPY'S ROOT AND IS SAVED (0283 point 2). A
// placed copy is saved as its root alone: its identity, transform, parent and
// prefab rows (0283 point 3). Nothing else of the copy reaches the file, so a
// saved prefab reaches every copy at the next read.
//
// THE PART ROW IS ON EVERYTHING MADE FROM A PREFAB, the root naming itself, and
// is never saved: it is registered runtime-only. The scene writer skips an
// entity whose part row names another.
//
// THE EDITOR EXPANDS (0283 point 4): a root with a prefab row and no part row is
// read from its file, its tree made, and every one given a part row. Nothing in
// this folder reads a file.
//
// PARTS CARRY IDENTITIES so the Scene list, a pick and undo's re-find by id work
// on them unchanged; only the part row says they are not the person's to edit.
//
// THE PREFAB STRUCT IS WRITTEN AS THE LIST OF ITS FIELDS (base/describe.h), so a
// build that asks for descriptions also has voe_scene_prefab_description().
#pragma once

#include <base/describe.h>
#include <base/imported.h>

#include <ecs/component.h>
#include <ecs/world.h>

// How many bytes a prefab's path holds, its terminator included.
#define VOE_SCENE_PREFAB_PATH 128

// The prefab file this root was placed from.
#define VOE_SCENE_PREFAB_FIELDS(F, F_READ_ONLY) \
	F(char, path, CHAR, VOE_SCENE_PREFAB_PATH)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_prefab, VOE_SCENE_PREFAB_FIELDS)

// The placed copy's root this entity was made for; the root names itself.
// Runtime-only, so it is not described.
typedef struct voe_scene_prefab_part {
	voe_ecs_entity instance;
} voe_scene_prefab_part;

// The keys these components are registered against. Their addresses are their
// identities.
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_prefab_key;
extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_scene_prefab_part_key;

// NULL when the world never registered the table, or the entity has no row. The
// pointer is into the table and is good until the next add or remove.
const voe_scene_prefab *voe_scene_prefab_get(const voe_ecs_world *world,
					     voe_ecs_entity entity);
const voe_scene_prefab_part *
voe_scene_prefab_part_get(const voe_ecs_world *world, voe_ecs_entity entity);
