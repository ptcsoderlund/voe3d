# 03 — One hue in every role, and an inverse to mark state
folder: ui
decisions: 0168, 0171, 0194, 0196

## Change
`ui/include/ui/theme.h`:
- `voe_ui_theme_inputs` gains `voe_math_float3 hue` (sRGB 0..1, the one colour a theme authors; its
  lightness is ignored, its hue and chroma tint every role). `accent` stays in the struct, read by nothing,
  with a one-line comment "unread; removed by card 10" — `theme` still writes it until card 07.
- `voe_ui_theme` gains `inverse` (a fill at `text_primary`'s lightness, what a held, dragged or selected
  control is drawn in) and `inverse_ink` (at `ground`'s lightness, the text on it). `accent` and
  `accent_ink` stay, set equal to `inverse` and `inverse_ink`, with "removed by card 10".
- Rewrite the header's paragraphs on the accent to say ADR-0194's rule: one hue, roles differ in
  lightness, chroma the same for all but where the gamut shrinks it, state shown by inversion.
- `voe_ui_theme_default_inputs` (declared here, defined in theme.c): `hue` is `#808080`, so Near black and
  Near white are pure grey.

`ui/src/theme.c`:
- Rename `ACCENT_CHROMA_MAX_DARK`/`_LIGHT` to `HUE_CHROMA_MAX_DARK`/`_LIGHT` (same values). Take OKLab
  `a`,`b` of `inputs->hue`; chroma C clamped to the mode's maximum; below 1e-4 it is zero.
- Replace `grey_rgba(l)` with a function giving role lightness `l` the hue: OKLab (l, a·k, b·k) where the
  scale k in 0..1 is the largest found by bisection (12 steps) for which the colour is in gamut — test "in
  gamut" as `voe_ui_oklab_from_linear(voe_ui_oklab_to_linear(lab))` within 1e-3 of `lab` on every channel,
  since `voe_ui_oklab_to_linear` clamps (see `ui/src/oklab.h`'s header). Every role goes through it:
  the ladder, border, three texts, `inverse` (text_primary's lightness), `inverse_ink` (ground's).
- Drop the accent's own lightness, `ACCENT_INK_SPLIT_L`, `INK_DARK_L`, `INK_LIGHT_L`. Update the file's
  header to match.

`ui/tests/theme.c`:
- Replace `test_only_accent_moves` with `test_hue_moves_only_tint`: changing `hue` changes no role's OKLab
  lightness (within 0.01).
- Add `test_every_role_shares_one_hue`: for both modes, both scalars at MIN, 1 and MAX, and hues `#D4A02B`,
  `#2B7FD4`, `#33AA33`, every role whose OKLab chroma exceeds 0.005 has hue angle `atan2(b, a)` within
  0.02 rad of the authored hue's, and no role's chroma exceeds the mode's maximum plus 0.002.
- Add `test_grey_hue_is_grey`: `#808080`, and the default inputs in both modes, give chroma under 0.002 in
  every role.
- `test_dark_clamps_chroma_harder_than_light` measures `surface`'s chroma instead of the accent's.
- `check_legible` also checks `inverse_ink` against `inverse` with the floor it uses for `text_primary`.

Update `ui/src/src.md`'s `theme.c` line and `ui/tests/tests.md`'s `theme.c` line.

## Done when
The folder's check passes (`checks.sh` for `ui`), with `test_every_role_shares_one_hue` and
`test_grey_hue_is_grey` called from `ui/tests/theme.c`'s `main`.
