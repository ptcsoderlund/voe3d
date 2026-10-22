# 70 — A glTF's ORM picture carries its texture times its factors
folder: assets
after: none
decisions: 0168, 0449

## Change
Bug 03: Import textures writes `tree asset-bark-orm.png` with blue 255 because
`voe_assets_model_orm` copies the metal-roughness picture's green and blue as
they are. Fixed once here, in the owner; its one caller,
`editor/src/model_import.c`, already writes the picture whole over the old one
on every Import textures, and needs no change. The signature stays.

`assets/src/model_material.c`, `voe_assets_model_orm`: where the material has
a metal-roughness picture, each written texel's green is that picture's green
times the material's `roughness` and its blue that picture's blue times its
`metallic`, rounded to the nearest whole value. Each factor is clamped to 0–1
first, as glTF allows no other and a file may still hold one (the file's
`clamp` serves). Red (occlusion) is unchanged. With no metal-roughness picture,
green and blue stay the factors × 255 as now. Size and nearest-texel sampling
stay as they are.

`assets/include/assets/model_material.h`, header paragraph THE ORM PICTURE:
the sentence that the picture's green and blue are the whole truth and the
factors not read (0401) goes; it now says green is the picture's green ×
roughness and blue its blue × metal, as glTF defines (0449), the factors
clamped to 0–1, and that a `.material` naming this picture therefore needs
nothing more, 0401's rule for the standard material standing. The comment on
`voe_assets_model_orm` itself needs no change unless it repeats the old rule.

`assets/src/model_material.c` file header: add the rounding point only if it
is not obvious from the code.

`assets/tests/model_material.c`: the existing ORM checks built from the 2×2
metal-roughness picture use factors that now scale it; read `gltf_material`
and `orm_model` and set the factors those checks need (1 where they mean "as
painted") or change their expected texels to the products, whichever keeps the
check's point. Add a test function `orm_multiplies_the_picture_by_the_factors`,
run from `main`, that checks on one built model:
- a material with `metallic` 0 over a picture whose blue is 255 gives blue 0
  in every texel and green the picture's green when `roughness` is 1 (the
  bark);
- a material with `metallic` 1 over blue 255 gives blue 255 (still fully
  metal);
- `metallic` 0.5 and `roughness` 0.5 over 200 give 100 in both;
- a `metallic` of 2 reads as 1.
Update the file's header list of cases with the factors.

`assets/tests/tests.md`, the `model_material.c` entry: gains the products by
the factors. `assets/src/src.md`, the `model_material.c` entry: the ORM picture
merged and multiplied by the factors. `assets/assets.md`, the
`model_material.h` entry: no change unless it states the old rule.

## Done when
- `orm_multiplies_the_picture_by_the_factors` and the existing checks in
  `assets/tests/model_material.c` pass under the folder's checks.
- `grep -n "factors are not read" assets/include/assets/model_material.h`
  prints nothing.
- The human, in the lumberhill project, runs Import textures again on
  `Assets/Lanscapestuff/tree asset.glb`: `tree asset-bark-orm.png` has blue 0
  everywhere and green matching `tree_bark_03_rough_4k.png`; the trunks on
  `Hill.landscape` look like matte wood in sun and shade, with no metal shine
  and no pink fill sheen.
