# 02 — A tree with flat triangles crashes the editor

## Seen
The editor crashes when I drag `spruce_ish` from the Assets panel into the scene view, in the lumberhill
project. It blocks step 1 of 090's test: the tree imported, but I cannot place or paint it.

The import itself went through: `Assets/trees/spruce_ish.glb` (16 MB, imported 2026-10-20 15:18), its
`.parts`, two materials and its levels file in `Cache/levels/` are all there.

## Reproduced
On Linux, debug build of `c2c0cd80`, with no window: a copy of the project whose `main.scene` holds
one more thing wearing `Assets/trees/spruce_ish.glb`, run as
`voe_editor <copy> --capture out.png --frames 10`. It aborts while the start loads the scene:

```
voe_editor: math/src/float3.c:61: voe_math_float3_normalize: Assertion `len > 0.0f' failed.
```

The stack (from a preloaded `__assert_fail` that prints `backtrace()`, symbolized):

```
voe_math_float3_normalize
walk_edges                      3d/src/shape_geometry.c:134
voe_3d_shape_geometry_build
build_shape                     3d/src/models.c:247
load_glb                        3d/src/models.c:359
voe_3d_models_load_levelled
load_changed                    game/src/models.c:358
voe_game_models_update
voe_editor_models_update
voe_editor_loading_start_work
starting_worker_run             game/src/starting.c:186
```

So it is not the drop: any load of this model crashes, the drop is just the first. A scene that holds it
will not open either.

## Why
`walk_edges` normalizes every triangle's face normal, the cross of two of its edges. The file has
triangles with no area — two corners on one point, or three on one line — and their cross is nought.
Counted from the file's own positions and indices:

| mesh | material | vertices | triangles | zero-area |
|---|---|---|---|---|
| 0 | Spruce_Leaves | 1,698 | 900 | 0 |
| 1 | Spruce_Bark | 2,676 | 4,097 | **190** |

Flat triangles are common in downloaded and exported models; one is not a broken file. Whatever loads a
model's triangles must take them: skip them, or give them no normal, and not assert. Other places
that make a normal per triangle (picking, the bake, simplify, the foliage brush's footing) may meet the
same triangles once this one is past.

## Also
This is the same assert as the Windows crash of 2026-10-01 (dragging the sun and changing water values
after 045), whose stack was lost. That one may have had another caller; this report does not claim it
fixed.
