// What Import writes beside a `.glb` it brings into Assets (0403, 0420 point
// 3): one `.material` per glTF material and the model's `.parts` naming them,
// so a placed model wears the engine's own materials and not glTF's; and what
// Import textures writes and dresses them in. assets_panel.c calls the first
// after the `.glb`'s copy succeeds, interface_assets.c the second:
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
// `Assets/…` path (assets/parts.h), keeping the levels of the `.parts` there,
// else the defaults (0430 point 4). The values are
// voe_assets_model_material_values's: colour, roughness, metal, shader,
// cut-out, both sides, emission, and no maps.
//
// EVERY IMPORT OVERWRITES BOTH. The rule is "the material comes from the
// `.glb`" (0403): importing again rewrites the materials from the file, an
// edit in the Inspector and a map assigned by hand included, and a re-export
// that reordered the materials is set right by it. No PNG is touched by it.
//
// IMPORT TEXTURES (0403, 0404, 0420 point 3), from the `.glb`'s right-button
// menu, writes every picture as `<stem>-<image name>.png` (the same rule,
// `image<N>`) and `<stem>-<material name>-orm.png` per material with an
// occlusion or metal-roughness picture (assets/model_material.h), each whole
// and atomically over a file of that name, no larger than `largest` on its
// longest side, aspect kept, never enlarged, so a run at a larger size gives
// back up to the `.glb`'s own size (0453); each shrunk by its use through
// voe_assets_image_fit (assets/image.h), normal, colour or data, as 0466
// point 3 says. Then each material the `.parts`
// names is read, or taken from the glTF values when it is gone, its
// `colormap`, `normalmap`, `ormmap` and `emissionmap` set to those PNGs, a
// slot the glTF material leaves empty kept, every other value kept, and
// written. No `.parts` says "Import the model first".
//
// REFUSED BEFORE ANYTHING IS WRITTEN, said in `why` naming the `.glb`: a file
// that will not read or parse, more than VOE_ASSETS_PARTS materials, a path
// holding `"` or a line break, which a file cannot quote, and a written
// path of VOE_ASSETS_MATERIAL_PATH bytes or more; for textures also a
// `.parts` that will not read or names a file not a `.material`. A write that
// fails is said naming its file, and nothing after it is written; those
// before it stand.
//
// Constraints: the whole file is parsed, its pictures decoded into `scratch`;
// for Import the pictures go unused, and a reader of the materials alone
// would lift that.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <stdbool.h>
#include <stdint.h>

// Writes `glb`'s materials and parts beside it, the parts keeping the levels
// of the one there, `glb` the project-relative
// `Assets/…` path of a `.glb` already in `<project_folder>/`. True when every
// file is written; false with `why` set. `scratch` is rewound.
[[nodiscard]] bool voe_editor_model_import(const char *project_folder,
					   const char *glb,
					   voe_base_arena *scratch,
					   voe_editor_notice *why);

// Import textures for `glb`, as above: its pictures written beside it and the
// materials its `.parts` names dressed in them, `largest` the longest side
// any written picture may have. True when every file is written; false with
// `why` set. `scratch` is rewound.
[[nodiscard]] bool voe_editor_model_import_textures(const char *project_folder,
						    const char *glb,
						    uint32_t largest,
						    voe_base_arena *scratch,
						    voe_editor_notice *why);
