# 10 — The accent leaves the palette
folder: ui
decisions: 0168, 0171, 0194, 0196

## Change
`ui/include/ui/theme.h`: remove `accent` from `voe_ui_theme_inputs` and `accent`, `accent_ink` from
`voe_ui_theme`, with every sentence about them; the header's remaining story is one authored `hue`, the
lightness ladder, the two scalars, and `inverse`/`inverse_ink` for state.

`ui/src/theme.c`: remove the two assignments and any constant left unused by them.

`ui/tests/theme.c`: remove any check reading `accent` or `accent_ink`.

`ui/ui.md`: change only the sentences that name the accent — the `include/ui/theme.h` entry says the one
hue and `inverse`/`inverse_ink`, and the folder's opening paragraph says one colour, two scalars, a mode
and a text size as before.

## Done when
The folder's check passes (`checks.sh` for `ui`) and `grep -rn accent --include=*.c --include=*.h
--include=*.md ui theme editor dev` prints only `theme`'s sentence about a refused `accent` line and
`dev`'s "accented character".
