# 11 — The glyph sheet holds √
folder: text
after: none
decisions: 0168, 0364

## Change
Bug 01: the Panels menu's tick √ (U+221A) draws as the missing-glyph box,
because the sheet holds only U+0020–U+00FF. Oxanium carries U+221A (its cmap
was read while planning).

`text/src/font.c`, and only it among the sources:
- Beside `FIRST_CHARACTER` / `LAST_CHARACTER`, a `static const uint32_t`
  array of the characters held beyond the run, in codepoint order, holding
  U+221A alone; its count as a constant. The slots: the run's, then one per
  listed character, then the missing-glyph box (`NOTDEF_SLOT` moves after
  them). The `glyphs` array in the font struct grows to match.
- Where the run's glyphs are rasterised into the sheet, the listed ones are
  too, each into its slot, the same way and with the same failure handling.
- `glyph_of`: a codepoint in the run as now; else a search of the list gives
  its slot; else the missing-glyph box.
- The comment over the range: the run plus the list (0364), why a list and
  not a wider run, and that a card needing another symbol adds an entry.
  The ATLAS_PIXELS comment ("which the range above fits in") stays true;
  name the list there.

`text/include/text/font.h`: read its header comments; where one says what the
sheet covers or what draws as the missing-glyph box, add that the listed
symbols are covered too. No signature changes.

`text/src/src.md`: the `font.c` entry names the list beyond Latin-1 as a
phrase, under the entry cap.

## Done when
- The folder builds and its tests pass.
- `grep -qi '0x221a' text/src/font.c` exits 0.
- `d=$(mktemp -d) && cmake --build --preset debug --target voe_editor && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0 (the sheet still builds and fits).
