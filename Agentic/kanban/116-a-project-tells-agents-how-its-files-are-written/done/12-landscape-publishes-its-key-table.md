# 12 — Landscape publishes its key table
folder: assets
after: 11
decisions: 0168, 0511, 0512

## Change
- `assets/include/assets/landscape.h`: declare
  `const voe_assets_documented_key *voe_assets_landscape_keys(uint32_t *count);`. Its comment points to
  the table and says the table holds the same sections and keys the header lists.
- `assets/src/landscape_keys.c` is new. `landscape.c` keeps its reader unchanged. Write one row
  (`voe_assets_documented_key`, from `assets/sectioned.h`) per key in landscape.h's header:
  - [Landscape]: size, cells;
  - [Layers]: layer<N>, grow<L>_<K>;
  - [Foliage]: kind<K>, size<K>, vary, turn, tilt, shadow, blend, blendheight;
  - [Heights]: row<R>;
  - [Paint]: paint<K>_<R>;
  - [Copies]: c<I>.

  For each row:
  - Take ranges from the header's constants (`SIZE_MIN`/`_MAX`, `CELLS_MAX`, `KIND_SIZE_MIN`/`_MAX`,
    `BLEND_HEIGHT_MIN`/`_MAX`, `GROW_MAX`), in the file's units, centimetres where the file stores
    centimetres.
  - The kind phrase says the shape of a row's value; a heights row, for example, is that many numbers.
  - Set `refused` from how `assets/src/landscape.c`'s strict read behaves. Read only the strict read
    and the helpers it calls.
- `assets/src/src.md`: add a line for `landscape_keys.c`. `assets/assets.md`: add the table's entry.
- `assets/tests/landscape_keys.c` is new; add its line to `assets/tests/tests.md`. It holds two tests:
  - `landscape_keys_ranges`: the rows for `size`, `cells`, `size<K>` and `blendheight` carry the header
    constants as least and greatest;
  - `landscape_keys_refused`: build the smallest landscape text that
    `voe_assets_landscape_read_strict` accepts, as landscape.h's header describes it. Set `size` to
    the row's greatest and the text reads. Set it above greatest and the text is refused when the row
    says `refused`. Do the same for `cells`.

  Do not read or extend `assets/tests/landscape.c` (906 lines).

## Done when
`ctest --test-dir build/debug -R '^assets/landscape_keys$' --no-tests=error` exits 0.
