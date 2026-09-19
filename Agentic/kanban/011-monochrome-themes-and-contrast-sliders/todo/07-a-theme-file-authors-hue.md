# 07 — A theme file authors `hue=`
folder: theme
decisions: 0168, 0172, 0194

## Change
`theme/src/theme.c`: the key list's `accent` becomes `hue`; `to_accent` becomes `to_hue` and writes
`theme->inputs.hue`; its refusal reads `line %u: hue '%s' is not #RRGGBB`. `accent` is then a key the schema
does not know and is refused at its line by the path that already refuses one — check that path names the
key and the line, and if unknown keys are not refused today, refuse them there.

`theme/include/theme/theme.h`: the example and the schema paragraph say `hue="#D4A02B"`, the one colour the
whole palette is tinted with, its lightness ignored (ADR-0194); `hue` is among the four required keys. Add a
sentence that `accent` is not read any more and a file that still has it is refused at that line, with no
alias (ADR-0194).

`theme/tests/theme.c`: every `accent=` in a fixture becomes `hue=`, and the checks on
`theme.inputs.accent` read `theme.inputs.hue`. Add `an_accent_line_is_refused`: a file whose colour line is
`accent="#D4A02B"` is refused, `voe_base_report_error_first()` naming that line's number.

## Done when
The folder's check passes (`checks.sh` for `theme`) and `grep -n accent theme/src/theme.c
theme/include/theme/theme.h` prints only the sentence about a refused `accent` line.
