# 03 — A picture gets its mips and a cooked form
folder: assets
after: none
decisions: 0168, 0470, 0466

## Change
0470 points 2 and 3: pictures carry a full mip chain, made on the CPU by the area average of their kind,
and a cooked picture is a head followed by its levels.

- `assets/include/assets/image.h`:
  - `voe_assets_image` gains `uint32_t levels`. 0 or 1 means `pixels` holds level 0 alone. n means n
    levels one after another, level i being `max(1, width >> i)` × `max(1, height >> i)` RGBA8. Zeroed, it
    means what every image means today.
  - `uint32_t voe_assets_image_level_count(uint32_t width, uint32_t height)`: the full chain down to 1×1.
  - `size_t voe_assets_image_size(const voe_assets_image *image)`: the bytes of all its levels.
  - `voe_assets_image voe_assets_image_mips(voe_base_arena *arena, const voe_assets_image *image, voe_assets_image_kind kind)`:
    a copy with the full chain. Level 0 is copied, and each next level is made from the one before by the
    area average of `kind`, exactly as `voe_assets_image_fit` shrinks. Asserts the input holds one level.
  - The cooked form: `#define VOE_ASSETS_IMAGE_COOKED_HEAD`, the head's size in bytes (≤ 64), and three
    functions:
    - `voe_assets_bytes voe_assets_image_cooked_write(voe_base_arena *arena, const voe_assets_image *image, voe_assets_image_kind kind, uint64_t key)`.
      It writes the head (magic, version, key, kind, width, height, levels), then every level, the total
      padded to 16.
    - `bool voe_assets_image_cooked_key(const uint8_t *head, size_t size, uint64_t *key)`. False when it
      is too short, or its magic or version is not this one's.
    - `bool voe_assets_image_cooked_read(const uint8_t *bytes, size_t size, voe_assets_image *image, voe_assets_image_kind *kind, uint64_t *key, size_t *used)`.
      It reads with no copy, `pixels` pointing into `bytes`, and `*used` is the padded size, so a
      containing file can read several in a row. False with no line when the magic, version or sizes are
      wrong: a stale or broken copy is made again, not reported.
  - The header's paragraphs say why levels sit back to back (0454 point 4), that a cooked picture is
    native-endian and for this machine's cache only, and that the version rises with any change to the
    layout.
- `assets/src/image_fit.c` (207 lines): the area-average shrink to an exact width and height moves behind
  a new internal header `assets/src/image_average.h`, as one function. Fit keeps its behaviour, and its
  `fit` keeps computing the target size.
- New `assets/src/image_levels.c`: the level count, size, mips, and the cooked write, key and read.
- `assets/src/src.md`: entries for `image_average.h` and `image_levels.c`. `assets/include/assets/assets.md`:
  the `image.h` entry names mips and the cooked form.
- New test `assets/tests/image_levels.c`, with these cases:
  - `level_count_follows_vulkan`: 1×1 is 1, 5×3 is 3, 256×1 is 9.
  - `mips_of_a_constant_stay_constant`: for every kind.
  - `mips_halve_a_checker_to_grey`: colour.
  - `cooked_round_trips`: kind, key, sizes and every byte come back.
  - `cooked_refuses_a_wrong_magic_or_short_bytes`.
  - `key_reads_from_the_head_alone`.

  And its entry in `assets/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^assets/image_(levels|fit)$'` passes.
