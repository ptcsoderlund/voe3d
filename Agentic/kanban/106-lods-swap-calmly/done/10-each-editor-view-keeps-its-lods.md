# 10 — Each editor view keeps its LODs
folder: editor
after: 07, 09
decisions: 0168, 0500, 0501

## Change
Each scene view and the preview hold one LOD memory (`3d/include/3d/lods_kept.h`; read its
header), begun each frame and set as the 3d frame's `lods` (card 06's field in
`3d/include/3d/draw_system.h`), so two views swap by their own cameras (0501 point 1).

- `editor/src/view.h`:
  - `voe_editor_view` gains `voe_3d_lods_kept *lods` beside `casters`: this view's memory, made by
    `voe_editor_views_create`, written only by 3d's draws.
  - `voe_editor_views` gains `preview_lods` beside `preview_casters`.
  - `voe_editor_views_create` gains `voe_base_arena *arena` (first after `views`): the memories are
    made on it, one for each of `VOE_EDITOR_VIEWS` and the preview, living as long as the arena.
    Its comment says so, with the memory's size from `lods_kept.h`; the struct's size note
    (~line 169) stays true, the memories being on the arena, not in the struct.
- `editor/src/view.c`: `voe_editor_views_create` (line ~106) makes them with
  `voe_3d_lods_kept_create(arena)`.
- `editor/src/main.c`: the call at line ~387 passes the persistent `arena` (the one made at ~291,
  not `scratch`).
- `editor/src/view_passes.c`: the preview's frame (~line 187) and each view's frame (~line 283):
  `voe_3d_lods_kept_begin(<its memory>, voe_platform_clock_now())` once, before that frame's
  shadows and draws, and `.lods = <its memory>` set as `.casters` is. Header comment notes each
  target keeps its own LOD memory on the wall clock (0501 point 2).
- `editor/src/src.md`: `view.h` and `view_passes.c` entries mention the LOD memory.

## Meaning
- Holds: each model copy and foliage copy keeps, per view, its LOD and the moment it last swapped;
  hysteresis per view (0495 pt 2, 0500 pt 1).
- Gaps answered by 0501: the editor's views run the dissolve on the platform steady clock, a paused
  world does not stop it; a view not drawn last frame sees its copies for the first time.

## Done when
`grep -q voe_3d_lods_kept_begin editor/src/view_passes.c` and
`grep -q voe_3d_lods_kept_create editor/src/view.c` exit 0, and the editor builds.
