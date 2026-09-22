# 13 — text carries Oxanium alone
folder: text
decisions: 0168, 0185

## Change
Pixel Operator leaves the folder (ADR-0185 replaces ADR-0167's second face).
- Delete `fonts/PixelOperator.ttf` and `fonts/PixelOperator-LICENSE.txt` with `git rm`.
- `include/text/font.h`: `voe_text_typeface` keeps its name and holds only `VOE_TEXT_TYPEFACE_OXANIUM`,
  so callers still name the face they make. Its comment says the set is the one face the binary carries,
  and that a theme naming any other font gets Oxanium (ADR-0185). Rewrite the "TWO FACES ARE IN THE
  BINARY" paragraph as one face, Oxanium, embedded with `#embed`, its licence the SIL Open Font License,
  its notice `fonts/OFL.txt` travelling unmodified beside it.
- `src/font.c`: remove `PIXEL_OPERATOR_TTF` and its `#embed`, `PIXEL_OPERATOR_ATLAS_PIXELS` and
  `_ATLAS_EM`, the switch case, and the comments that exist for Pixel Operator (its CC0 notice, "the same
  range is asked of both faces", "sixty-four for Pixel Operator", its glyph size beside Oxanium's). Keep
  the per-face `atlas_em` / `atlas_pixels` pair only if the code still reads cleanly with one face; the
  fewer names, the better.
- `src/truetype.c`: keep reading `COMPONENT_HAVE_SCALE`; reword the two comments that cite Pixel Operator
  so they give the reason without naming it (a font may scale a component, and a scale of −1 mirrors one).
- `tests/truetype.c`: delete the four `*_pixel_operator` checks. `check_both_faces_at_once` becomes
  `check_two_fonts_at_once`, opening Oxanium twice at once, so the no-leaking-static claim still holds.
- `text.md`, `src/src.md`, `tests/tests.md`: name one face and one licence; `fonts/` holds Oxanium Regular
  and `OFL.txt`.

## Done when
`git grep -n -i -e "pixel.\?operator" -e PixelOperator -- text` prints nothing, `ls text/fonts` lists
exactly `OFL.txt` and `Oxanium-Regular.ttf`, and the folder check in `CLAUDE.md` with `{folder}` = `text`
exits 0 (`text/truetype` passes).
