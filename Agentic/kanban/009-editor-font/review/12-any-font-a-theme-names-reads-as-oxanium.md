# 12 — Any font a theme names reads as Oxanium
folder: theme
decisions: 0168, 0185

## Change
`src/theme.c`: the `font` key is still optional and still known, but its value is no longer checked.
Whatever it holds, `oxanium`, `pixel_operator` or anything else, `typeface` is
`VOE_TEXT_TYPEFACE_OXANIUM` and nothing is reported (ADR-0185). Remove the `pixel_operator` branch and
the refusal of an unknown font. Change nothing else in the reader.

`include/theme/theme.h`: the example file says `font=oxanium`. The paragraph on optional keys says
`font` may name anything and always reads as Oxanium, the one face the engine carries, with no report
(ADR-0185). Remove "not a known font" from the list of refusals. Correct `theme.md` if it says otherwise.

`tests/theme.c`:
- `a_good_file_gives_every_value` keeps `font=pixel_operator` in its text and now expects
  `VOE_TEXT_TYPEFACE_OXANIUM`.
- In `every_refusal_names_its_line`, the `font=comic_sans` case goes.
- New `any_font_reads_as_oxanium`: `font=pixel_operator` and `font=comic_sans`, each in an otherwise good
  file, read true with `VOE_TEXT_TYPEFACE_OXANIUM`, and after `voe_base_report_error_clear` before the read,
  `voe_base_report_error_first()` returns NULL. Call it from `main`, and list it on `tests/tests.md`.

## Done when
`git grep -n -i "pixel.\?operator" -- theme` prints only the two test lines that feed `font=pixel_operator`,
and `ctest --test-dir build/debug -R "^theme/"` passes after `cmake --build --preset debug --target voe_theme`
and the theme tests are built (the folder check in `CLAUDE.md` does both).
