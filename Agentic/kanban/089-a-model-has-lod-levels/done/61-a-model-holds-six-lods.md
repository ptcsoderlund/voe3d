# 61 — A model holds six LODs, and old levels files are made again
folder: 3d
after: 58, 60
decisions: 0168, 0425, 0426, 0430, 0433

## Change
0433: up to 6 levels in all (`VOE_ASSETS_LEVELS` is 6 since card 60), and every levels file made
before is stale, since a file does not record its count and the default was 4.

- `3d/include/3d/models.h`: `VOE_3D_MODEL_LEVELS` 6. The device-room comment: four parts for each
  of the 128 files at each of six levels, plus two, so `VOE_3D_MODELS_GEOMETRIES` 3074; the levels
  now aim down to a thirty-second, so the "seven eighths more" phrase becomes under thirty-one
  thirty-seconds more. `VOE_3D_MODELS_VERTICES` and `_INDICES` stay.
- `3d/include/3d/model_levels.h`: THE LEVELS says `levels.count` is 1 to 6; the own-levels
  paragraph says `_LOD1` to `_LOD5`; THE LAYOUT is version 4; STALE names versions 1 to 3, and a
  phrase that 4 changes no field, only remakes the files made when the default count was four
  (0433).
- `3d/src/model_levels.c`: `VERSION` 4. Nothing else.
- `3d/src/model_bake.h`: the node-level paragraph says a node of n above 5 (VOE_3D_MODEL_LEVELS
  less one) is left out. `3d/src/model_bake.c`: that limit taken from `VOE_3D_MODEL_LEVELS`
  (`3d/models.h`), not a literal 3, in the walk and in `_levels`.
- `3d/tests/model_levels.c`: case 8 also sets the version to 3: read false. New case 16: the
  sphere made with `VOE_ASSETS_LEVELS_DEFAULT` reads `level_count` above 4. Header lists it.
- `3d/tests/model_bake.c`: a new check: a root "Rock" with a child "Rock_LOD5" (one shared
  triangle each, as the file's level case builds them) has `_levels` 6; "Twig_LOD7" stays left
  out. Header names it.
- `3d/tests/models_levels.c` and `3d/tests/draw_levels.c`: any device room or comment that counts
  four levels counts `VOE_3D_MODEL_LEVELS` (the "TWELVE GEOMETRIES" paragraph becomes sixteen,
  `.geometries` written `2 * VOE_3D_MODEL_LEVELS + 2 + 2`).

Leave `3d/tests/tests.md` (card 58's). `game` and `editor` follow in cards 62 to 64.

## Done when
The tests `3d/model_levels` (case 16 in it), `3d/model_bake`, `3d/models_levels` and
`3d/draw_levels` pass after the folder's build.
