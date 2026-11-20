// Reference text: how a project's files are written, as Markdown, for a reader —
// an agent — who writes those files by hand (0511 point 2).
//
//     voe_authoring_text scene;
//     uint32_t count;
//     const voe_assets_documented_key *keys = voe_assets_material_keys(&count);
//     voe_authoring_text material;
//
//     if (!voe_authoring_reference_scene(world, arena, &scene))
//             ...                     // reported; no text
//     (void)voe_authoring_reference_keys("Material files", keys, count, arena,
//                                        &material);
//
// IT IS GENERATED FROM THE TABLES THE READERS READ BY, SO IT SAYS WHAT THE STRICT
// READ ENFORCES. The scene text comes from the world's registered component
// descriptions (base/describe.h), with their ranges, names, menus, needs and
// former names; a key text from a format's own key table (assets/sectioned.h).
// Neither restates a rule by hand, so neither can promise a key the reader
// refuses. Ranges are in the file's units.
//
// WHAT EACH HOLDS:
//
//   - voe_authoring_reference_scene: `## The text form` — what `[N]` and
//     `[N.<type key>]` are, that a key not listed is refused with its line, and
//     that a section of a type nothing registered passes unread — then one
//     `## <type key>` per registered type that is not runtime-only, in key
//     order: its menu, needs and former names where it has them, and a table
//     with one row per field — key, kind, range and unit, and what a missing
//     key reads as, spelled as the scene writer spells it.
//   - voe_authoring_reference_keys: `## <title>` and one table row per key —
//     section, key, kind, default or "required", range and unit, and what it
//     says — then a line on what a refused range means.
//
// Nothing here opens a file: the text lands in the arena, and where it goes is
// the caller's.
#pragma once

#include <assets/sectioned.h>
#include <authoring/scene_write.h>
#include <base/arena.h>
#include <ecs/world.h>

#include <stdint.h>

// The scene text for every type registered in `world`. On success `*out` holds
// it. On failure — a described type whose description this build compiled out,
// or a missing value the writer cannot spell — it returns false, reports why
// and leaves `*out` untouched, as scene_write.h does. The arena holds the text
// and this call's working memory; what is pushed is the caller's to rewind.
[[nodiscard]] bool voe_authoring_reference_scene(const voe_ecs_world *world,
						 voe_base_arena *arena,
						 voe_authoring_text *out);

// The text for one key table, `count` rows of `keys`, under `## <title>`. Every
// row can be written, so it returns true and fills `*out`; the bool is the shape
// the scene text shares. The arena holds the text.
[[nodiscard]] bool voe_authoring_reference_keys(
	const char *title, const voe_assets_documented_key *keys, uint32_t count,
	voe_base_arena *arena, voe_authoring_text *out);
