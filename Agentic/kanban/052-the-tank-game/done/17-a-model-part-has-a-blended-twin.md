# 17 — Every model part in the store has a blended twin record
folder: 3d
after: 16
decisions: 0168, 0277, 0336

## Change
0336 point 2; nothing draws with it yet (card 18 does). Read `3d/include/3d/models.h`,
`3d/src/models.c`, the headers of `3d/src/model_upload.h` and `3d/src/model_picture.h`,
`3d/src/model_picture.c`, the material upload's comment in `3d/include/3d/material_component.h`,
`3d/tests/models.c`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`.

- `models.h`: `voe_3d_model_part` gains `voe_render_shading faded` after `material`, commented:
  the record the part is drawn with while its row fades. `VOE_3D_MODELS_SHADINGS` becomes 1025;
  its comment counts two records a part. A header paragraph: each `.glb` part not already
  BLENDED gets a twin, its material with the alpha mode BLENDED, uploaded at load beside its own
  (so uploads stay between frames) and freed with it on a replace, a failed load's give-back and a
  clear; a blended part's, a picture's and the water's `faded` is its own `shading`.
- `models.c`: the load makes each part's twin after the part's own record; a refused twin upload
  is the load's REFUSED failure, its records given back as today. The replace, the failed load's
  give-back and `_clear` free each twin that differs from its `shading`. The water part's `faded`
  is its `shading`.
- `model_picture.c`: both picture parts' `faded` is their own `shading`.
- `3d/tests/models.c`: its device capacity gains room for the twins (the comment says so). New
  checks: a loaded `.glb` part's `faded` differs from its `material.shading`; a picture entry's
  parts' `faded` equal their `shading`; a replace then a clear still leave the device able to load
  the model again (no twin leaked).
- `3d/include/3d/3d.md` (`models.h`), `3d/src/src.md` (`models.c`, `model_picture.c`),
  `3d/tests/tests.md` (`models.c`): each entry gains the twin, a phrase, where it no longer says
  what the file does.

## Done when
`ctest --test-dir build/debug -R '^3d/models$'` passes with the twin checks; on a machine
without a graphics card it skips, and building `voe_test_3d_models` is the proof.
