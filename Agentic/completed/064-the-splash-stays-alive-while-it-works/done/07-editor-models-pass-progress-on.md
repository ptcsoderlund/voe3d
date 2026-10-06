# 07 — The editor's model store passes progress on
folder: editor
after: 06
decisions: 0168, 0370

## Change
- `editor/src/models.h`, `editor/src/models.c`: `voe_editor_models_update` gains a last parameter
  `voe_game_progress *progress` (`game/progress.h`), handed to `voe_game_models_update`; NULL
  outside a splash wait. The header says a worker may call it during a wait (0370 point 4).
- `editor/src/main.c`: its one call passes NULL.
- `editor/src/src.md`: the `models.h` entry mentions progress.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.
