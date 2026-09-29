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
// WHAT A PREFAB MAY HOLD IS NOT CHECKED BY THE WRITER. No camera, light, prefab
// or part row is the caller's to check before it writes, and the reader's when
// it reads (0283 points 6 and 8).
//
// READING ONE IS A LOAD, rule 3's creation exception (0283 point 4), as
// authoring/scene_read.h is: it adds rows straight to the entities it has just
// made and to the root it is expanding, asks no system and submits no intent.
// It never overwrites a row the root has, and never touches another entity.
//
// COOKING ONE QUEUES STRUCTURE ADDS, NOT COMPONENT ADDS (0283 point 9): a spawn
// lands mid-step, from game logic, so its rows go through ecs/structure.h and
// appear at the step's structural apply. NO IDENTITY IS COOKED: spawned things
// are not authored, and the identity table stays at 32 (0283 point 11). The
// caller makes the entities and owns the include line.
#pragma once

#include <authoring/scene_write.h>
#include <base/arena.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

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

// Reads `size` bytes of prefab text onto `root`, which must be alive with a
// transform, in a world that registered identities and prefab parts. The text
// is validated whole first, as voe_authoring_scene_read does, and also refused
// without exactly one `[N]` lacking a parent section, or holding a camera,
// light, prefab or part section; refused, it creates nothing. Then `root` gets
// every row of the file's root it lacks, never identity, transform or parent;
// every other entity is made in ascending file id, its identity the file's name
// with id `first_id`, `first_id + 1`, ...; an ENTITY naming a file id names the
// entity made for it (the file's root: `root`), any other is zero; and `root`
// and every made entity get a part row naming `root`. `*out_next_id` is the id
// after the last used. A section of an unregistered type is skipped, one
// warning per type. The arena holds working memory, the caller's to rewind.
// A world out of room returns false half-made, as voe_authoring_scene_read.
[[nodiscard]] bool voe_authoring_prefab_read(const char *text, size_t size,
					     voe_ecs_world *world,
					     voe_ecs_entity root,
					     uint64_t first_id,
					     voe_base_arena *arena,
					     uint64_t *out_next_id);

// Cooks `world`, holding one prefab read by voe_authoring_scene_read, into one
// definition with no includes: `static bool <function>(voe_ecs_world *world,
// const voe_ecs_entity *entities, voe_math_double3 position, voe_math_quat
// rotation)`. `entities[0]` is the root, the one entity with no parent row; the
// rest follow ascending by id. Each entity's described, not runtime-only rows
// but its identity are queued by voe_ecs_structure_add, false when one is
// refused; the root's transform is `position`, `rotation` and scale one; an
// ENTITY naming a prefab entity is `entities[j]`, any other a zeroed entity.
// `*out_entities` is how many entities it needs. Refuses, arena and `*out` as
// voe_authoring_scene_cook, and a world without exactly one root.
[[nodiscard]] bool voe_authoring_prefab_cook(const voe_ecs_world *world,
					     const char *function,
					     voe_base_arena *arena,
					     voe_authoring_text *out,
					     uint32_t *out_entities);
