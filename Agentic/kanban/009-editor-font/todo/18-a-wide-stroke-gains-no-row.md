# 18 — A wide stroke gains no row, and the feature closes

folder: render/tests
decisions: 0168, 0182, 0183, 0184, 0212, 0177

## Change
One claim added to `render/tests/elements.c`, headless, using the bar sheet and the glyph placement
the two claims from card 10 already have (read that file's header for how a sheet is made and a
glyph placed in pixels; reuse the bar helper rather than writing a second one).

- `a_wide_stroke_gains_no_row`: a bar 4 texels tall — every texel's three channels
  `0.5 + (2 - |y - centre|) / (2 * 4)` clamped to 0..1 and stored as a byte, as in card 10 — drawn
  at one texel per screen pixel with its centre a quarter of a pixel off a pixel centre, so the true
  outline falls between pixel centres. Read back: the middle column holds exactly 4 rows of the
  glyph's colour, not 5. With ADR-0184's fixed half-pixel dilation it was 5; with ADR-0212 the
  thinnest stroke is already two pixels at this scale, so the cut does not move.
- Both of card 10's claims stay and keep passing: `thin_stroke_keeps_a_pixel` (small text still
  dilates) and `aligned_stroke_keeps_its_width`.
- Update the `elements.c` entry in `render/tests/tests.md` to name the new claim, and its count of
  claims needing no graphics card if that changes.

## Done when
- `cmake --build --preset debug --target voe_render && ctest --test-dir build/debug -R '^render/'`
  passes, with all three bar claims in `elements.c`.
- Putting `0.5` back as the dilation in `voe_render_element_cutoff` locally makes
  `a_wide_stroke_gains_no_row` fail and the other two pass (check once, then restore).
- `./build/debug/editor/voe_editor --capture /tmp/009-bug05.png --size 1920x1080` exits 0 and writes
  the file; leave it for the human.
- `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

For the human, on the sponsor's machines:
1. Open the capture above at 100 percent and measure the "P" of "Preferences": its stem is 2 or 3
   pixels at a cap height of about 23, not 4, and the top bar reads slim on the desktop monitor.
2. Start `./build/debug/editor/voe_editor` on the desktop machine: panels, Scene list, Inspector,
   file browser and Preferences are Oxanium at the face's weight, in both themes.
3. On the Fedora KDE laptop, start the same build and shrink the window in steps: no letter loses a
   stroke — the "T" of "Theme's own", "Close" and the theme names stay whole — edges stay hard with
   no grey, and full-screen text looks no worse than before.
