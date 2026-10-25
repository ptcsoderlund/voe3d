# 09 — A queue compresses maps a slice at a time
folder: game
after: 08
decisions: 0168, 0386, 0456, 0470, 0477

## Change
0477 points 3 and 4: the queue that compresses maps between frames and writes their copies. Card
10 fills it and card 11 steps it.

- `game/include/game/compressing.h` (new) — `voe_game_compressing`:
  - `voe_game_compressing *voe_game_compressing_new(void)` and `void voe_game_compressing_destroy(
    voe_game_compressing *)`.
  - `void voe_game_compressing_add(voe_game_compressing *, const char *folder, const char *path,
    voe_assets_image_kind kind, uint64_t stamp, const voe_assets_image *chain)`. It copies an RGBA8
    chain into the queue's own memory, and replaces a waiting job of the same path and kind. Past
    `VOE_GAME_COMPRESSING` (16) jobs it drops the map with one line.
  - `const char *voe_game_compressing_step(voe_game_compressing *, double seconds, voe_base_arena
    *scratch)`. It encodes the oldest job's blocks (`voe_assets_image_blocks_encode`) in runs of 64,
    reading `voe_platform_clock_now` between runs, until `seconds` pass or the job ends. A step always
    encodes at least one run. An ended job is written with `voe_game_models_picture_write`, keyed by
    its stamp, and freed, and its path is returned: the queue's copy, valid until the next call. NULL
    when none ended.
  - `uint32_t voe_game_compressing_waiting(const voe_game_compressing *)` and
    `void voe_game_compressing_clear(voe_game_compressing *)`.
  - The header says: one thread, the owner's; between frames; the memory is the queue's (rule 11's
    exception, as the store's is), about 28 MB for a 2048² job; why not a worker (0456).
- `game/src/compressing.c` (new) — the above.
- `game/tests/compressing.c` (new; no card needed):
  - a 64×64 colour chain is added and stepped with 0 seconds until a path comes back; its copy reads
    back BC7, keyed by the stamp, through `voe_game_models_picture_take`;
  - a second add of the same path and kind replaces the first;
  - clear leaves none waiting.
  - It makes its folder as `game/tests/models_cooked.c` does.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — entries.

## Done when
The `game/compressing` test passes.
