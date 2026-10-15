# 05 — A `.glb` gets its levels file
folder: 3d
after: 04
decisions: 0168, 0425, 0426

## Change
0425 point 1 makes the levels; 0426 point 1 is the file they are kept in. New public header, since
`game` (card 09) makes and reads the file and the store (card 06) reads it.

- `3d/include/3d/models.h`: `#define VOE_3D_MODEL_LEVELS 4` beside `VOE_3D_MODEL_PARTS`, one comment
  line naming 0425. Nothing else here.
- `3d/include/3d/model_levels.h` (new; includes `<3d/models.h>`): header comment carrying 0425
  point 1 and 0426 point 1 as a reader needs them (targets, the stop at a quarter of the radius, the
  three-quarters rule, own-levels files simplified never, the layout below, stale never an error),
  a usage example, and:
  - `voe_3d_model_levels`: `bool own`; `uint32_t level_count` (1 when own or none made);
    `uint32_t part_count`; `float errors[VOE_3D_MODEL_LEVELS]` (0 nought);
    `uint32_t *indices[VOE_3D_MODEL_LEVELS][VOE_3D_MODEL_PARTS]` and
    `uint32_t index_counts[...][...]`, level 0's unused: the bake's part indices are level 0.
  - `uint64_t voe_3d_model_levels_hash(const uint8_t *bytes, size_t size)`: FNV-1a 64.
  - `bool voe_3d_model_levels_make(voe_base_arena *arena, const uint8_t *glb, size_t glb_size,
    const uint8_t **bytes, size_t *size, voe_base_error *error)`: parse, bake level 0, simplify,
    write the file's bytes; all on `arena`, the caller rewinds. False, `error` set, as the store's
    load would fail (MALFORMED, UNSUPPORTED, REFUSED).
  - `bool voe_3d_model_levels_read(voe_base_arena *arena, const uint8_t *bytes, size_t size,
    const uint8_t *glb, size_t glb_size, voe_3d_model_levels *out)`: false when stale (magic,
    version, hash, a count past its limit, cut short, bytes left over); indices copied onto `arena`.
- `3d/src/model_levels.c` (new): the above. Layout: magic "VOEL", version 1, the hash, own,
  level count, part count, all `uint32_t` but the hash; then per made level k ≥ 1 its error and per
  part its count and indices. Own: `voe_3d_model_bake_levels` above 1 gives a one-level own file.
  Else bake level 0 (`3d/src/model_bake.h`), radius = farthest vertex from the box centre; level k's
  target per part a whole number of triangles near its count >> k, `max_error` radius / 4, each
  from level 0 with `voe_3d_simplify` (`3d/src/simplify.h`); the level's error the largest of its
  parts'; a level over three quarters of the previous level's triangles ends the list.
- `3d/src/src.md`: the `model_levels.c` entry. `3d/3d.md`: the `include/3d/model_levels.h` entry.
- `3d/tests/model_glb.inc` (new): a static helper building, on an arena, a `.glb` of a UV sphere
  (rings, segments as arguments; positions, normals and indices; one material, one node "Ball"),
  and with a flag a second node "Ball_LOD1" wearing the same mesh. Write the JSON and BIN chunks as
  `assets` reads them (`assets/include/assets/model.h`'s header says what it accepts).
- `3d/tests/model_levels.c` (new), a plain program, no graphics card:
  1. a 64 × 32 sphere: make, then read with the same glb true; at least 3 levels; each level's
     triangles at most three quarters of the one before; errors above nought and non-decreasing;
  2. read with one glb byte changed, with the bytes cut by 4, and with the version changed: false;
  3. the sphere with "Ball_LOD1": read gives `own` and one level;
  4. the hash of "a" is 0xaf63dc4c8601ec8c;
  5. make of bytes that are no glb: false, MALFORMED.
  Its header lists the five.
- `3d/tests/tests.md`: entries for both new files.

## Done when
The test `3d/model_levels` passes after the folder's build.
