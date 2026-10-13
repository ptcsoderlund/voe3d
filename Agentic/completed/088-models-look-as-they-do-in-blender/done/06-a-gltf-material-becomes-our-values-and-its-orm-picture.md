# 06 — A glTF material becomes our values and its ORM picture
folder: assets
after: 05
decisions: 0168, 0401, 0403, 0420

## Change
The pure half of Import and Import textures (0420 points 4 and 5), so the editor only names files and
writes them (cards 17 and 21).

- `assets/include/assets/model_material.h` (new, header comment): a glTF material as a `.material`
  file's values, and the ORM picture Import textures writes for it; why BLEND becomes a cut-out, why
  only a uniform colour scale becomes repeat, and the ORM rule, citing 0420 and 0401.
  - `voe_assets_material_file voe_assets_model_material_values(const voe_assets_material *material)`:
    0420 point 4: shader, colour, roughness, metal, `repeat` from an even `colour_scale` above 0 else 1,
    `cutout`, `double_sided`, `emission`, `emission_strength`; every map "".
  - `[[nodiscard]] bool voe_assets_model_orm(const voe_assets_model *model, uint32_t material,
    voe_base_arena *arena, voe_assets_image *out)`: 0420 point 5's picture in `arena`; false when the
    material names neither an occlusion nor a metal-roughness picture. Asserts `material` is in range.
- `assets/src/model_material.c` (new, header comment): the two.
- `assets/tests/model_material.c` (new, built `voe_assets_model` values in the test, no file): opaque,
  MASK 0.3, BLEND, unlit, double-sided, emissive strength and an uneven scale map to the expected
  values; ORM from one 2×2 metal-roughness picture alone has red 255; with a 4×4 occlusion picture it
  is 4×4, red from occlusion, green and blue from the 2×2 at its nearest texel; occlusion alone takes
  green and blue from the factors; neither is false.
- `assets/assets.md`: an entry for the new header; `assets/src/src.md` and `assets/tests/tests.md`: one
  each (each at most 300 characters).

## Done when
The test `assets/model_material` passes after the folder's build.
