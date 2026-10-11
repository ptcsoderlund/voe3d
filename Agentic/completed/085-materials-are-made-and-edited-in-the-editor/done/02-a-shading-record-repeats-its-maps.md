# 02 — A shading record repeats its maps
folder: render
after: none
decisions: 0168, 0399

## Change
0399 point 3: the record's `reserved_c` word becomes `float uv_repeat`.

- `render/include/render/device.h` — in `voe_render_shading_values`, `reserved_c` becomes
  `uv_repeat`, same offset and size; its comment says the mesh's texture coordinates are multiplied by
  it before any of the five maps is read and before `base_colour_uv_rect`, and that 0 reads as 1 so a
  zeroed record is unchanged.
- `render/shaders/` — `bindings.slangh`'s shading record renames
  the word, and the fragment stage multiplies the uv once, by `uv_repeat` or 1 when it is 0, before every
  map read in `draw.slang` and any part that samples a map.
- `render/src/records_layout.c` — the layout proof names the new member.
- `render/src/descriptors.c` — only if its asserts name `reserved_c`.

Update the header comment of the record and `render/shaders/shaders.md` where they say what the
fragment stage reads.

Test: add a case to `render/tests/surface_maps.c` (read its header for how a frame is drawn and read
back): a quad whose base colour texture is left half black, right half white, drawn with `uv_repeat` 4,
reads alternating black and white eight times across one row of the picture; with 0, twice.

## Done when
`ctest --test-dir build/debug -R '^render/surface_maps$'` passes with the new case.
