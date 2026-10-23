# 04 — The model store uploads given mips
folder: 3d
after: 02, 03
decisions: 0168, 0470

## Change
A picture that carries its levels (`voe_assets_image.levels` > 1, card 03) is uploaded with them through
`voe_render_texture_create_levels` (card 02). A picture with no levels goes through
`voe_render_texture_create` SMOOTH as today. Nothing else in the upload changes.

- `3d/src/model_upload.c`: the one texture create (near line 31) picks between the two by the image's
  `levels`.
- `3d/src/models_material.c`: the same at its texture create (near line 124).
- `3d/src/model_upload.h`: a sentence in the header that a picture with levels keeps them.
- `3d/include/3d/models.h`: the comments of `voe_3d_models_put_material` ("uploaded mipped") and
  `voe_3d_models_put` say that given levels are uploaded as they are and the GPU mips the rest.
- `3d/tests/models_material.c`: add a case `put_material_takes_given_levels`. A 64×64 colour map is made
  with `voe_assets_image_mips` and put. The put is true, and the entry is loaded with one part. It skips
  without a graphics card, as the file's other cases do.
- `3d/tests/tests.md`: the `models_material.c` entry names the case.

## Done when
`ctest --test-dir build/debug -R '^3d/models_material$'` passes with `put_material_takes_given_levels` in
it.
