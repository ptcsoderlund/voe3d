# 08 — A map's copy is compressed
folder: game
after: 07
decisions: 0168, 0470, 0477

## Change
0477 points 1, 3 and 8 for `Cache/pictures/<path>.<kind>`. Every caller that cooks a map's copy goes
through `voe_game_models_picture_save`, so after this card they all write it compressed. The
materials call, the game's materials call and the editor's cook before Play and Ship are such callers.

- `game/src/models_cooked.h`, `game/src/models_cooked.c`:
  - `voe_game_models_picture_save` compresses the chain after the mips, by kind
    (`voe_assets_image_compress`). It writes the compressed chain and returns it, and the caller
    uploads that.
  - New: `void voe_game_models_picture_write(const char *folder, const char *path, const
    voe_assets_image *chain, voe_assets_image_kind kind, uint64_t stamp, voe_base_arena *arena)`.
    It writes a chain as given as the map's copy, with its folders made, atomically, a failed write
    being a line on stderr. This is the end of `_save`, split out for card 09.
  - The header's notes: copies are compressed; the arena cost of a save.
- `game/include/game/models.h` — the COOKED COPIES and MATERIALS paragraphs: a copy's pictures and
  maps are compressed (0477).
- `game/tests/models_cooked.c`:
  - map copies written by the materials call read back BC7 for colour and ORM and BC5 for normal;
    so do those written by `voe_game_models_cook`;
  - a map copy of the old version counts stale.
- `game/src/src.md`, `game/tests/tests.md` — the `models_cooked` entries.

## Done when
The `game/models_cooked` test passes.
