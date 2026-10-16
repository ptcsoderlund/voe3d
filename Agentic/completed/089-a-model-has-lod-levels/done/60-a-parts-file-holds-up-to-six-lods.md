# 60 — A parts file holds up to six LODs, six by default
folder: assets
after: none
decisions: 0168, 0430, 0433

## Change
0433: a model has 1 to 6 levels in all, default 6. The keys and type names stay.

- `assets/include/assets/parts.h`: `VOE_ASSETS_LEVELS` 6; `VOE_ASSETS_LEVELS_DEFAULT`'s count 6;
  the header's example `count=6`; the `[Levels]` paragraph says `count` is 1 to 6, default 6
  (0433). The comment on `VOE_ASSETS_LEVELS` keeps "equal to `3d`'s VOE_3D_MODEL_LEVELS" (card
  61 raises that one; until then `3d`'s static_assert fails, which is expected).
- `assets/src/parts.c`: the refusal message for a count out of range says 1 to 6 (it reads
  "not 1 to 4" now). The range check already follows `VOE_ASSETS_LEVELS`; a count is still one
  digit.
- `assets/tests/parts.c`: `count=5` is now valid: the malformed case becomes `count=7`; a new
  check that `[Parts]`, `[Levels]`, `count=6` reads count 6; any check of a default count
  written as 4 reads 6 (or `VOE_ASSETS_LEVELS_DEFAULT.count`). The header's list follows.

## Done when
The test `assets/parts` passes after the folder's build, with the `count=6` and `count=7` checks
in it.
