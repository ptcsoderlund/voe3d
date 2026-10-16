# 73 — The loader carries the parts file's swaps into the levels file
folder: game
after: 68, 72
decisions: 0168, 0435, 0436

## Change
0436 point 7: a swap-only change in the `.parts` reaches a fresh levels file without a
remake, so the editor reloads instead of preparing (cards 75, 76); and the triangle counts
the LODs panel shows.

- `game/src/models.c`:
  - `levels_asked` also says whether a `.parts` was there and read (an out `bool`).
  - `levels_for`: when the Cache file is fresh and not to be remade, and a `.parts` read
    whose `swaps[1..5]` differ from the file's, a copy of the bytes on scratch gets
    `voe_3d_model_levels_swaps_set` (`3d/include/3d/model_levels.h`) and is written back
    as a made file is (a failed write a line on stderr), and that copy is handed on. With
    no `.parts` (a shipped game) the file's own swaps stand. `*made` stays false.
    Update, watch, prepare and reload all pass through it; check that each does.
  - New `voe_game_models_levels_triangles` (below): reads `<folder>/<path>` and, when
    there, `<folder>/Cache/levels/<path>.levels`, and calls
    `voe_3d_model_levels_triangles`; never makes or writes a file; scratch rewound.
  - Header comment: the patch of the swaps.
- `game/include/game/models.h`:
  - `uint32_t voe_game_models_levels_triangles(const char *folder, const char *path,
    voe_base_arena *scratch, uint32_t triangles[VOE_3D_MODEL_LEVELS], bool *own,
    bool *fresh)`: the number of levels whose triangles it wrote, and `*fresh`, as the 3d
    call; 0 when the `.glb` will not
    read. For the editor's LODs panel (0435 point 4).
  - LEVELS paragraph: the file is made with the `.parts`' count, strength, shares and
    swaps; a fresh file whose swaps differ from the `.parts`' has them written in, not made
    again (0435 point 4). Keep the header comment at 60 lines or less
    (`checks.sh --header-lines`).
- `game/tests/models_levels.c` (card 68 made it):
  - `check_levels_asked`'s `swap="far"` step checks `grid_read(...).swaps[k]` 2 for k 1 to
    5 in place of `.swap`.
  - New step: `swap2=3` with the other keys absent, prepared with remake: `swaps[2]` 3, the
    others 1.
  - New case `check_swaps_patched`: the grid prepared with the defaults; `grid.parts`
    rewritten with `swap1=4` only; a prepare without remake makes none (`made` false), and
    the file on disk reads `swaps[1]` 4 and the same index counts as before. On a world and
    store: the grid updated (its file at ×4), then `grid.parts` given `swap1=0.5`, `voe_game_models_reload`:
    the entry's `levels[1].bound` is 8 times what it was, within 1e-5 relative.
  - New case `check_triangles`: the grid prepared with the defaults: the call returns its
    file's level count, `own` false, `fresh` true, level 0 the grid's 128 triangles, each level fewer than
    the one before; `missing.glb`: 0.
  - Header comment lists the new steps and cases. Raise `CAPACITIES` if the new store needs
    room, said in its comment.
- `game/tests/tests.md`: the `models_levels.c` entry names the swaps patched and the
  triangle counts.

## Done when
The tests `game/models_levels` and `game/models` pass after the folder's build, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/include/game/models.h`
prints 60 or less.
