# 02 — Pixel Operator beside Oxanium, and a font asked for by typeface
folder: text
decisions: 0167, 0168

## Change

Done on 2026-09-17 under the earlier workflow as task 2 of spec 006; converted to a card by ADR-0168.

Build ADR-0167. Add `PixelOperator.ttf` (Regular) and its CC0 licence text to
`text/fonts/`, unrenamed and unmodified, beside Oxanium and its OFL. Add
`voe_text_typeface` — `VOE_TEXT_TYPEFACE_OXANIUM`, `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` — and make
it `voe_text_font_new`'s first argument; both faces are `#embed`ded and one font stays one face,
one atlas, one weight, no fallback. Check Pixel Operator against `ATLAS_EM` as ADR-0167 requires
— its design grid is 0.0625 em — and, if the sampling has to move for that face alone, move it
for that face alone and put the reason in `src/font.c` beside the existing one. Update the call
sites the signature breaks: `dev/src/main.c`, `editor/src/main.c`, `ui/tests/widgets.c`, and
`text`'s own tests, each naming the face it draws in. Extend `tests/truetype.c` to read the new
face as it reads Oxanium, and check that both faces' fonts can exist at once.

## Done when

`cmake --build --preset debug && ctest --test-dir build/debug -R '^(text|ui)/'` — all tests pass, and `voe_dev` and `voe_editor` still link.
