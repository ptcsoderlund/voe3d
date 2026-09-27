# 01 — The element test splits by what it claims
folder: render
decisions: 0168, 0173

## Change
`render/tests/elements.c` is 1573 lines and card 02 changes its glyph claims. Split it by
function, moving code only: no claim is added, dropped or reworded, no expected value changes.
Each test file is its own program (every `tests/*.c` is built as one), so shared code goes in a
header, not a `.c`.

- New `render/tests/element_scene.h`: everything two or more of the programs below use — the
  readback resolve, the camera and light, the `solid`/`glyph` record builders, the field sheet
  texels and sheet rectangles, the colour constants, `read_back`, `colour_of`, `count_in`,
  `open_frame`, `close_frame`, `struct scene` and the device setup and skip that `main` does now.
  Each function `static inline` (or `[[maybe_unused]] static`) so a program that leaves one
  unused still builds under `-Werror`. Its header comment: why a test helper is a header here, and
  that it includes `../src/device_internal.h` for the reason `elements.c` gave.
- `render/tests/elements.c` keeps the rectangle claims: colours, clip, paint order, blend,
  overrun, a mesh after an element draw, an empty frame, no element room, two ranges, a range past
  what was submitted. Its header lists those.
- New `render/tests/glyphs.c`: the letter claims — `a_solid_and_a_glyph_are_one_draw`,
  `a_glyph_reads_the_sheet`, `a_glyph_is_clipped_like_a_solid`,
  `a_glyph_with_no_sheet_draws_a_solid_rectangle`, `paint_order_holds_across_kinds`, `bar_sheet`
  and the three stroke claims (thin, aligned, wide), with their header comments. Its header: the
  claims it makes and why a bar sheet is written as `text/src/raster.c` writes one.
- New `render/tests/element_transform.c`: the claims needing no graphics card — the transform's
  origin, the near clip boundary, the surface matrix, the surface size, `ui` scale at two, the
  eighty-byte record. Its header says it needs no device.
- `CAPACITIES` / `MAX_ELEMENTS` go where they are used; a program whose claims need fewer
  elements may keep the same numbers.
- `render/tests/tests.md`: the `elements.c` entry narrows to rectangles; new entries for
  `glyphs.c`, `element_transform.c` and `element_scene.h`.

Each of the three `.c` files ends under 800 lines.

## Done when
- `cmake --preset debug && cmake --build --preset debug --target voe_render $(ninja -C build/debug -t targets all | grep -oE "^voe_test_render_[A-Za-z0-9_]+")` exits 0.
- `ctest --test-dir build/debug -R '^render/'` passes, and `render/elements`, `render/glyphs`
  and `render/element_transform` are among the tests it runs.
- `wc -l render/tests/elements.c render/tests/glyphs.c render/tests/element_transform.c` shows
  each under 800.
- The claim functions are all still called: `grep -c` of each name listed above across the three
  files is at least 2 (defined and called).
