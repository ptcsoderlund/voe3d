# 29 — A prepare can make a model's levels again
folder: game
after: none
decisions: 0168, 0426, 0429

## Change
0429 point 3: the editor's Make levels (card 30) makes a `.glb`'s levels file even when the one
there is fresh. `voe_game_models_prepare` gains the flag; the loader keeps a fresh file as now.

- `game/include/game/models.h`: `voe_game_models_prepare` gains `bool remake` after `scratch`:
  false keeps a fresh file as now; true makes and writes it whatever is there, `made` then true
  whenever the `.glb` reads and parses. Its comment says so. The LEVELS paragraph's sentence on
  _prepare names its two uses: the editor's import (fresh kept) and its Make levels (0429, made
  again).
- `game/src/models.c`: the static helper shared by `_prepare` and `load_changed` takes `bool
  remake`; true skips the read of the existing file and goes straight to the making and the
  write. `load_changed` passes false; `_prepare` passes its argument. Nothing else changes.
- `game/tests/models.c`: the levels case's existing prepare calls pass false. After its stale
  check, one more: prepare of `GOOD` with remake true on the fresh file gives true, `made` true,
  and the file read again fits the `.glb` (`voe_3d_model_levels_read`). The header's line on the
  case names the remake.
- `game/tests/tests.md`: the `models.c` entry names a remake in its phrase on the levels file.

The one caller outside the folder, `editor/src/loading.c`, is card 30's; leave it.

## Done when
The test `game/models` passes after the folder's build, with the remake check in it.
