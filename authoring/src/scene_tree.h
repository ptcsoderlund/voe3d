// The scene writer's walk narrowed to one tree — how prefab_write.c reuses
// scene_write.c's walk rather than copying it, and what value_write.c asks when
// it spells an ENTITY inside that tree.
//
// A TREE IS A ROOT AND THE ENTITIES UNDER IT, the root among them. Given one, the
// walk writes only the tree's authored entities, leaves out the root's parent
// section, writes the root's transform at the origin with no turn and scale one,
// and writes an ENTITY naming outside the tree as 0 with no warning (0283
// point 1). Given none, it writes the whole world exactly as scene_write.h says.
//
// MEMBERSHIP IS A LINEAR SCAN of `entities`, once per identity and once per
// ENTITY value. A prefab is a few dozen things; a copy sorted by entity index
// with a binary search would lift it for trees of thousands.
#pragma once

#include <authoring/scene_read.h>
#include <authoring/scene_write.h>
#include <base/arena.h>
#include <ecs/world.h>

#include <stdint.h>

typedef struct {
	voe_ecs_entity root;
	// The root and everything under it, `count` of them, in any order.
	const voe_ecs_entity *entities;
	uint32_t count;
} voe_authoring_tree;

// True when `entity` is one of the tree's.
bool voe_authoring_tree_holds(const voe_authoring_tree *tree,
			      voe_ecs_entity entity);

// voe_authoring_scene_write, narrowed to `tree` — NULL for the whole world —
// with the same arena contract and refusals.
[[nodiscard]] bool voe_authoring_scene_write_tree(
	const voe_ecs_world *world, const voe_authoring_kept *kept,
	const voe_authoring_tree *tree, voe_base_arena *arena,
	voe_authoring_text *out);
