# 33 — A parts file keeps a model's levels
folder: assets
after: none
decisions: 0168, 0420, 0430

## Change
0430 point 4: `.parts` gains an optional `[Levels]` section. The type lives here, below `3d`, so the
store's maker, the loader and the editor share it.

- `assets/include/assets/parts.h`:
  - `#define VOE_ASSETS_LEVELS 4`, the most levels in all (equal to `3d`'s `VOE_3D_MODEL_LEVELS`).
  - `typedef enum { VOE_ASSETS_LEVELS_GENTLE, VOE_ASSETS_LEVELS_MEDIUM, VOE_ASSETS_LEVELS_STRONG }
    voe_assets_levels_strength;` and `typedef struct { uint32_t count; voe_assets_levels_strength
    strength; } voe_assets_levels;`, each field commented (count 1 to VOE_ASSETS_LEVELS, level 0
    included).
  - `#define VOE_ASSETS_LEVELS_DEFAULT ((voe_assets_levels){ 4, VOE_ASSETS_LEVELS_MEDIUM })`.
  - `voe_assets_parts` gains `voe_assets_levels levels;` after `materials`.
  - header: the example gains the section (`count=4`, `strength="medium"`); a paragraph on `[Levels]`:
    optional, each key absent read as its default, what the two keys mean in a phrase each (0430
    points 2 and 3), and that the loader reads it when making a levels file (0430 point 5), so the
    "game never reads this" sentence names that one exception. `_read`'s comment: MALFORMED also for a
    count outside 1–4, a strength not one of the three words, any other key in `[Levels]`, any other
    section. `_write`'s comment: always writes `[Levels]` after `[Parts]`; asserts the count in range.
- `assets/src/parts.c`: the read takes the optional section through the sectioned reader as it takes
  `[Parts]`, filling `VOE_ASSETS_LEVELS_DEFAULT` first; the strength a quoted word, as paths are
  quoted; the count a whole number. The write prints the section. Header: one sentence on the
  section.
- `assets/tests/parts.c`: the existing round trip sets `levels` to a non-default (2, STRONG) and still
  matches byte for byte; new checks: text with no `[Levels]` reads the default; `[Levels]` with only
  `strength="gentle"` reads count 4, GENTLE; `count=0`, `count=5`, `strength="hard"` and `[Levels]`
  `colour=1` each refused MALFORMED. The header lists them.
- `assets/tests/tests.md`: the `parts.c` entry names the levels. `assets/src/src.md`: the `parts.c`
  entry names the `[Levels]` section. `assets/assets.md`: the `parts.h` entry says a model's levels too.

## Done when
The test `assets/parts` passes after the folder's build, with the new checks in it.
