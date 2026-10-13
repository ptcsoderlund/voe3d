// A glTF material as a `.material` file's values, and the ORM picture Import
// textures writes for it (0420 points 4 and 5). Pure: the editor names the
// files and writes them; nothing here opens one.
//
// THE VALUES (0420 point 4): `unlit` under KHR_materials_unlit, else `lit`;
// colour the base colour's rgb; roughness and metal the factors; the emissive
// factor and strength; `doubleSided`; every map "", since maps are Import
// textures' job (0403). A cutout above 1 or a negative emission, which glTF
// does not allow but a file may still hold, is clamped so the written file
// reads back.
//
// BLEND BECOMES A CUT-OUT AT A HALF. The engine has no see-through (0404), and
// downloaded foliage is often marked BLEND: drawn opaque, its cards would be
// squares. MASK keeps its cutoff; OPAQUE is 0.
//
// ONLY A UNIFORM COLOUR SCALE BECOMES `repeat`. `repeat` is one float; an
// uneven scale, an offset or a rotation would need a two-float repeat and a new
// record, file and Inspector, which nothing asks for (0420). Those read as 1.
//
// THE ORM PICTURE (0420 point 5): red is the occlusion picture's red, else 255;
// green and blue are the metal-roughness picture's, else roughness and metal ×
// 255. glTF packs both pictures this way already, so this only merges them
// when they are two. It is the larger of the two by pixel count, each read at
// its nearest texel. With a metal-roughness picture its green and blue are the
// whole truth and the factors are not read (0401).
#pragma once

#include <assets/image.h>
#include <assets/material.h>
#include <assets/model.h>
#include <base/arena.h>

#include <stdint.h>

// The `.material` values Import writes for `material`; no map is set.
voe_assets_material_file
voe_assets_model_material_values(const voe_assets_material *material);

// The ORM picture of `model`'s material number `material`, its pixels in
// `arena`. False, `out` untouched, when the material names neither an
// occlusion nor a metal-roughness picture. Asserts `material` is in range.
[[nodiscard]] bool voe_assets_model_orm(const voe_assets_model *model,
					uint32_t material, voe_base_arena *arena,
					voe_assets_image *out);
