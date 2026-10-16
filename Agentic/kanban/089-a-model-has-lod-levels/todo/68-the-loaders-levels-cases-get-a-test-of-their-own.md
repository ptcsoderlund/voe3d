# 68 — The loader's levels cases get a test of their own
folder: game
after: none
decisions: 0168

## Change
`game/tests/models.c` is 936 lines, and the next cards add levels cases to it. Split it by
function before they do; no case changes what it checks.

- New `game/tests/glb_build.h`: the hand-built `.glb` helpers both tests use, moved out of
  `models.c` as `static` definitions the way `render/tests/element_scene.h` holds its own:
  `put_u32`, `glb_pack`, `JSON`, `TRIANGLE`, `glb_build`, and the world helpers `place` and
  `wear`. A header comment: what the helpers build, and that each test includes it once.
- New `game/tests/models_levels.c`: the levels cases moved out of `models.c` — the `GRID_*`
  defines, `GRID_JSON`, `grid_build`, `levels_fit`, `grid_read`, `grid_prepared`,
  `check_levels_asked`, `grid_held`, `check_reload`, `check_levels` — with their own
  `FOLDER` (`game_models_levels_test`) and the paths under it, their own `CAPACITIES` (the
  current one, `geometries` at `VOE_3D_MODEL_LEVELS + 2`, with its comment), their own
  `remove_files` and a `main` as `models.c`'s: headless device, skip without a graphics card,
  the folder made and `good.glb` written, `check_levels` then `check_reload`, files removed.
  Its header comment carries `models.c`'s THE LEVELS paragraph, said of this file.
- `game/tests/models.c`: those functions, defines and the THE LEVELS paragraph go; it
  includes `glb_build.h`; its `main` and `remove_files` lose the levels calls and paths.
  Keep its `CAPACITIES` unless the build warns.
- `game/tests/tests.md`: the `models.c` entry no longer names levels; one-sentence entries
  for `models_levels.c` (the levels file and the `.parts` it is made with, prepare and
  reload) and `glb_build.h`.

## Done when
The tests `game/models` and `game/models_levels` pass after the folder's build;
`wc -l < game/tests/models.c` prints under 800; `grep -c 'check_levels\|GRID' game/tests/models.c`
prints 0.
