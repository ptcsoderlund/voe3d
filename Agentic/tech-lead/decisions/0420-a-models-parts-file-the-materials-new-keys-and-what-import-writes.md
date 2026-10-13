# 0420 — A model's `.parts` file, the material's new keys, and what Import writes
date: 2026-10-13
by: planner

## Decision
For 088, carrying out 0403 and 0404 where they leave the shape to the planner.

1. **The model's file is `<stem>.parts`** beside `<stem>.glb`: sectioned text, one `[Parts]` section,
   one key `material<N>` per glTF material, N its index in the file, the value a quoted project-relative
   `.material` path ("" for none). Parsed and written in `assets/parts.h`, at most 32 materials. A scene
   row still names the `.glb`; the store gives each baked part the path its glTF material's key names.
   A `.glb` with no `.parts`, or a part whose path names no loaded material, draws with the file's own
   glTF material as before, so hand-copied files and the tank game are unchanged. The game reads no
   `.parts` (0236): the cook writes them into `materials.c` as `voe_game_parts_cooked` beside the
   materials. A rename or move of a `.material` is followed inside `.parts`; a `.glb`'s rename, move,
   duplicate and trash take its `.parts` along.
2. **The material file gains five keys**: `cutout` (0–1, 0 none: below it a pixel is not drawn),
   `double_sided` (`true`/`false`, default false), `emission` (three linear floats, default 0 0 0),
   `emission_strength` (default 1) and `emissionmap` (a path, COLOUR). Emission added is emission ×
   strength × the map.
3. **What Import writes**: `<stem>-<name>.material` per glTF material, `<name>` the material's name with
   every byte outside `A–Z a–z 0–9 - _` made `_`, `material<N>` when it has none, `-2`, `-3`… on a
   collision within the file; then `<stem>.parts`. Both overwrite what is there. **Import textures**
   writes every picture as `<stem>-<image name>.png` (same rule, `image<N>`) and one
   `<stem>-<material name>-orm.png` per material with occlusion or metal-roughness; it reads `.parts` to
   find the materials, sets `colormap`, `normalmap`, `ormmap` and `emissionmap` and keeps every other
   value; a material file that is gone is written again from the glTF values with the maps.
4. **glTF to values**: `lit`, or `unlit` under KHR_materials_unlit; colour the base colour's rgb;
   roughness and metal the factors; MASK's cutoff as `cutout`, BLEND as cut-out at 0.5, OPAQUE 0;
   `doubleSided`; the emissive factor and KHR_materials_emissive_strength. KHR_texture_transform on the
   colour texture: a uniform scale becomes `repeat`; offset, rotation and an uneven scale are not carried.
   These three extensions are read even when required. Wrap modes are not read (REPEAT), and
   magnification stays 0359's NEAREST.
5. **The ORM picture** per material: red the occlusion picture's red, else 255; green and blue the
   metal-roughness picture's, else roughness and metal × 255; at the larger of the two sizes, each read
   at its nearest texel.

## Reasoning
- The row keeps naming the `.glb`: every scene, prefab, drag and cook already carries that path.
  Naming the `.parts` instead would change all of them for no look.
- Index keys and not names: sectioned text refuses a repeated key or section, and glTF names repeat and
  hold any byte. A re-export that reorders materials is set right by importing again, as 0403 says.
- Fallback to the glTF material: refusing to draw a `.glb` without `.parts` would break every model
  copied in by hand before 088.
- BLEND as cut-out: 0404 adds no see-through; downloaded foliage is often marked BLEND, and opaque
  would draw its cards as squares.
- One ORM picture per material, always: one rule, one name; a shared picture written twice is bytes.
- Texture transform and wrap modes in full: a two-float repeat and an offset change the record, the file
  and the Inspector, and nothing in 088's test asks for them.

## Replaces
Nothing. Amends 0399 point 1 (the file's keys) and point 6 (model rows lose `materials`).
