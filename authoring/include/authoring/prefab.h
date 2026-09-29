// One tree in a world, written as the text of a `.prefab` file (ADR-0283).
//
// A PREFAB FILE IS SCENE TEXT HOLDING ONE TREE (0283 point 1): exactly one entity
// with no parent, its root, and everything under it (scene/parent_component.h's
// voe_scene_parent_tree), in authoring/scene_write.h's order and spelling. Its
// ids are local to the file; they are written as they are in the world, and
// whoever reads the file maps them onto its own.
//
// THE ROOT'S TRANSFORM IN THE FILE IS WHERE IT SITS WHILE THE PREFAB IS OPEN, AND
// NOTHING ELSE. No placed copy and no spawn takes it, so it is written at the
// origin with no turn and scale one, whatever it is in the world, and the
// root's parent section is left out.
//
// WHAT CROSSES OUT OF THE TREE IS NOT IN THE FILE. An ENTITY naming outside the
// tree is written `0`, with no warning: that is what a prefab is. Kept sections
// are not written.
//
// WHAT A PREFAB MAY HOLD IS NOT CHECKED HERE. No camera, light, prefab or part
// row is the caller's to check before it writes, and the reader's when it reads
// (0283 points 6 and 8).
#pragma once

#include <authoring/scene_write.h>
#include <base/arena.h>
#include <ecs/world.h>

// Writes `root` and everything under it as prefab text into `arena`. On success
// `*out` holds the text. On failure it returns false, reports why naming the
// entity, and leaves `*out` untouched. It refuses a root that is not alive or
// has no identity, and everything voe_authoring_scene_write refuses. The arena
// contract is voe_authoring_scene_write's: working memory beside the text, and
// what is pushed, on success or failure, is the caller's to rewind.
[[nodiscard]] bool voe_authoring_prefab_write(const voe_ecs_world *world,
					      voe_ecs_entity root,
					      voe_base_arena *arena,
					      voe_authoring_text *out);
