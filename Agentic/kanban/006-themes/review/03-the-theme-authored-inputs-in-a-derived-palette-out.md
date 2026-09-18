# 03 — The theme: authored inputs in, a derived palette out
folder: ui
decisions: 0170, 0171, 0168

## Change

Done on 2026-09-17 under the earlier workflow as task 3 of spec 006; converted to a card by ADR-0168.

New `include/ui/theme.h` and `src/theme.c`, plus internal `src/oklab.h` / `src/oklab.c`.
`voe_ui_theme_inputs` is the authored set of ADR-0097 — `accent` as authored sRGB,
`contrast_strength`, `surface_separation`, `mode` (light or dark) — plus `text_size` in
millimetres per em. `voe_ui_theme` is the derived palette: ground, surface, raised surface,
hairline border, control, control hovered, three text lightnesses, accent, ink on accent, each
linear RGBA, beside the `const voe_text_font *` the caller chose and the text size. Derivation is
`voe_ui_theme_derive(const voe_ui_theme_inputs *, const voe_text_font *)`, all of it in OKLab per
ADR-0171: sRGB in, lightness steps sized by the two scalars, hue never rotated, every text role
stepped until it clears its surface, the accent's chroma clamped harder in dark than in light.
`voe_ui_theme_default_inputs()` is the built-in near-black theme with a blue accent. No widget
and no existing function changes in this task. Tests in `tests/theme.c`: a round trip through
OKLab, that both modes stay legible at both ends of both scalars, that only the accent moves when
only the accent moves, that each scalar moves what it names and nothing else, and the dark/light
chroma asymmetry. Needs no graphics card — a NULL font is allowed here and asserted on only where
a label is measured.

## Done when

`cmake --build --preset debug --target voe_ui && ctest --test-dir build/debug -R '^ui/'` — all tests pass.
