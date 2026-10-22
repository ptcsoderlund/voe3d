# 01 — A picture is shrunk to a largest side
folder: assets
after: none
decisions: 0168, 0453, 0466

## Change
0466 points 1–3: one call that hands back a decoded picture no larger than a given side. Nothing
calls it yet; card 02 does.

- `assets/include/assets/image.h` — add, after `voe_assets_png_encode`:
  - `typedef enum { VOE_ASSETS_IMAGE_COLOUR, VOE_ASSETS_IMAGE_NORMAL, VOE_ASSETS_IMAGE_DATA }
    voe_assets_image_kind;`
  - `voe_assets_image voe_assets_image_fit(voe_base_arena *arena, voe_assets_image image,
    uint32_t largest, voe_assets_image_kind kind);` — `image` itself, pixels untouched and
    nothing pushed, when its longest side is at most `largest`; else a new picture in `arena`
    sized by 0466 point 1. Asserts `largest` ≥ 1 and pixels not NULL (rule 13).
  - Header points: the size rule and that nothing is enlarged (0453); why an area average (0466
    point 2); what each kind does to its texels (point 3), in particular that a normal is made unit
    length again and colour is averaged in linear light and weighted by alpha; the working memory
    is one row of floats, never a float picture; row zero stays the top (the file's no-flip rule).
- `assets/src/image_fit.c` (new) — the size, the per-output-row accumulation over the source rows
  its footprint covers with fractional edge weights, the sRGB decode table and encode, the normal
  decode, renormalise and encode, the alpha-weighted colour mean. Header: what the file holds.
- `assets/tests/image_fit.c` (new) — named checks:
  - sizes: 4096×2048 at 1024 is 1024×512; 1×4096 at 512 is 1×512; 300×200 at 512 hands back the
    same pixels pointer and size; 3×3 at 2 is 2×2.
  - a constant 3×3 picture at 2 is that constant in every texel, for each kind (weights sum to 1).
  - colour: a 2×2 checker of sRGB 0 and 255, all opaque, at 1 is about 188 (linear mean), not 128.
  - colour: opaque red beside transparent green, 2×1 at 1, is red with alpha 127 or 128.
  - normal: the encodings of (1,0,1)/√2 and (−1,0,1)/√2 side by side at 1 decode to within a
    step of (0,0,1), unit length.
  - data: 0 beside 255 at 1 is 127 or 128 in every channel.
- `assets/assets.md`, `assets/include/assets/assets.md` — `image.h`'s entry says a picture is also
  shrunk to a largest side. `assets/src/src.md`, `assets/tests/tests.md` — an entry for each new
  file.

## Done when
Test `assets/image_fit` passes.
