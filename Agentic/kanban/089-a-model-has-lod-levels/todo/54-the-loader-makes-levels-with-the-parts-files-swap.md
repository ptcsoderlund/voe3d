# 54 — The loader makes levels with the parts file's swap
folder: game
after: 46, 50
decisions: 0168, 0430, 0432

## Change
0432 point 2: the loader already hands the `.parts`' whole `voe_assets_levels` to
`voe_3d_model_levels_make`, so the swap (`assets/include/assets/parts.h`, card 48) reaches the
levels file (`3d/include/3d/model_levels.h`, card 50) with no code change. Say so, and prove it.

- `game/include/game/models.h`: the sentence at line ~23 says the count, strength and swap of the
  `.parts`; nothing else in the header changes (it is at its cap).
- `game/tests/models.c`: in the levels case that writes `grid.parts` (card 36's, its step 2), a
  further step: `grid.parts` written as `[Parts]`, `[Levels]`, `swap="far"`; prepare with remake:
  the file read (`voe_3d_model_levels_read`) has `swap` 2. The header's LEVELS paragraph names the
  swap in a phrase, within its cap.
- `game/tests/tests.md`: the `models.c` entry names the swap in a phrase, still one sentence.

## Done when
The test `game/models` passes after the folder's build, with the swap step in it.
