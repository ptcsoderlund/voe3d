# 04 — game hands the panel a levels summary
folder: game
after: 03
decisions: 0168, 0500

## Change
Card 03 replaced `voe_3d_model_levels_triangles` with `voe_3d_model_levels_summarise` and
`voe_3d_model_levels_summary` (`3d/include/3d/model_levels.h`; read its comment). game's wrapper
follows. Its caller, editor's `levels_ask.c`, is fixed by card 09; this card does not touch it.

- `game/include/game/models.h`: `voe_game_models_levels_triangles` becomes
  `void voe_game_models_levels_summarise(const char *folder, const char *path,
  voe_base_arena *scratch, voe_3d_model_levels_summary *out)`; same rules (the levels file in the
  Cache when there, never made or written, scratch rewound), `out->count` 0 when the `.glb` will
  not read.
- `game/src/models.c`: the function at line ~49 renamed and calling `_summarise`.
- `game/tests/models_levels.c`: the checks at ~325–340 use the new call and check the same counts,
  plus that a fresh file's `errors[1]` is above nought and `radius` above nought.
- `game/include/game/game.md`: entry says summary.

## Done when
The test `game/models_levels` passes and
`! grep -rq voe_game_models_levels_triangles game` exits 0.
