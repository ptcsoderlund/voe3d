# 49 — A material's ORM map fills the roughness and the occlusion slots
folder: 3d
after: 48
decisions: 0168, 0400

## Change
0400: a `.material`'s third map is an ORM picture. It is uploaded once, as DATA, and its one texture
fills both the record's `metallic_roughness_texture` (green roughness, blue metal, times the
sliders) and its `occlusion_texture` (red, read by the shader on the fill and bounce only, card 47).
An empty `ormmap` leaves both slots none: occlusion 1, roughness and metal from the sliders.

- `3d/include/3d/models.h` — in `voe_3d_material_maps`, `roughness` becomes `orm`. The comment on
  `voe_3d_models_load_material` says the ORM map is uploaded as DATA into the metal-roughness and
  occlusion slots both (0400), in place of "roughness in the metal-roughness slot (0399 point 2)".
- `3d/src/models_material.c` — the field renames of card 48 (`colormap`, `normalmap`, `ormmap`);
  in `upload_material` the ORM map's one texture is set in both slots. It is one texture made, so it
  is held and freed once; `MAPS` stays 3. Header: the ORM map's two slots and why one upload.
- `3d/tests/models_material.c` — the field renames; `check_loaded` checks `occlusion_texture` is the
  same index as `metallic_roughness_texture` and not none; the load with only a colour map leaves
  both none.
- `3d/src/src.md`, `3d/3d.md`, `3d/include/3d/3d.md` — any line saying "roughness map".

`game` and `editor` name the old names; cards 50 and 51 rename those.

## Done when
`ctest --test-dir build/debug -R '^3d/(models_material|draw_material)$'` passes.
