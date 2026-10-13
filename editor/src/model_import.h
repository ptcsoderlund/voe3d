// What Import writes beside a `.glb` it brings into Assets (0403, 0420 point
// 3): one `.material` per glTF material and the model's `.parts` naming them,
// so a placed model wears the engine's own materials and not glTF's.
// assets_panel.c calls it after the `.glb`'s copy succeeds:
//
//     if (!voe_editor_model_import(project, "Assets/Trees/oak.glb", scratch,
//                                  &why))
//             ...                     // why names the file
//
// THE NAMES (0420 point 3): `<stem>-<name>.material` beside the `.glb`,
// `<name>` the glTF material's name with every byte outside `A–Z a–z 0–9 - _`
// made `_`, `material<N>` when it has none, and `-2`, `-3`… on a collision
// within the file, compared in any case, since a case-blind disk would make
// two of them one file. Then `<stem>.parts`, `material<N>` naming each by its
// `Assets/…` path (assets/parts.h). The values are
// voe_assets_model_material_values's: colour, roughness, metal, shader,
// cut-out, both sides, emission, and no maps.
//
// EVERY IMPORT OVERWRITES BOTH. The rule is "the material comes from the
// `.glb`" (0403): importing again rewrites the materials from the file, an
// edit in the Inspector and a map assigned by hand included, and a re-export
// that reordered the materials is set right by it. No PNG is touched here;
// pictures are Import textures' alone.
//
// REFUSED BEFORE ANYTHING IS WRITTEN, said in `why` naming the `.glb`: a file
// that will not read or parse, more than VOE_ASSETS_PARTS materials, a path
// holding `"` or a line break, which a parts file cannot quote, and a
// material's path of VOE_ASSETS_MATERIAL_PATH bytes or more. A write that
// fails is said naming its file, and nothing after it is written; those
// before it stand.
//
// Constraints: the whole file is parsed, its pictures decoded into `scratch`,
// to read its materials; a reader of the materials alone would lift that.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <stdbool.h>

// Writes `glb`'s materials and parts beside it, `glb` the project-relative
// `Assets/…` path of a `.glb` already in `<project_folder>/`. True when every
// file is written; false with `why` set. `scratch` is rewound.
[[nodiscard]] bool voe_editor_model_import(const char *project_folder,
					   const char *glb,
					   voe_base_arena *scratch,
					   voe_editor_notice *why);
