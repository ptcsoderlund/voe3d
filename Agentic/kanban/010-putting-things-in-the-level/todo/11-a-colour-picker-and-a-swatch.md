# 11 — A colour picker and a swatch
folder: ui
decisions: 0168, 0192, 0177

## Change
New `include/ui/colour.h` / `src/colour.c`: both widgets are 0192's, and any panel can use them.
- `voe_ui_node voe_ui_swatch(voe_ui_context *ui, voe_math_float3 linear, voe_ui_sizing sizing)`: one solid
  element of that colour, alpha 1, and no input. A caller that wants it clickable puts it inside a button.
- `voe_ui_node voe_ui_colour_picker(voe_ui_context *ui, const char *name, uint32_t index, voe_math_float3
  linear)`: a panel with a 32 × 32 mm saturation/value square (20 × 20 solid cells), a 32 × 4 mm hue strip (36
  cells), a marker on each at the current values, and a hex field (card 09's field) showing `#RRGGBB`.
  Hue, saturation and value are HSV of the sRGB colour. The context remembers them for this key while the
  picker is made every frame, so hue survives a grey. A press in the square or strip, and a drag started there,
  sets S/V or H and so the colour, every frame.
- The hex field accepts `#RRGGBB` or `RRGGBB`, any case, on commit. Anything else is refused: the colour stays,
  and a label "not #RRGGBB" shows until the next edit.
- `typedef struct { bool changed; voe_math_float3 value; bool outside; bool refused; } voe_ui_colour_result;`
  and `voe_ui_colour_result voe_ui_colour_picker_action(const voe_ui_context *ui, voe_ui_node picker)`.
  `value` is linear. `outside` means the primary button went down this frame outside the picker's rectangle.
  Closing on it, or on Escape, is the caller's choice.
- sRGB ↔ linear: reuse whatever `src/oklab.c` or `src/theme.c` already has for it; do not write a second.
- The header says why the picker is placed by the caller and not a popup, why the gradients are cells, and
  what the element and node cost is, so a caller can size its capacities.

Update `ui.md`, `src/src.md` and `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ui`) with new `tests/colour.c`: a swatch emits one element of its
colour; a press at the square's top-right corner (S=1, V=1) with hue 0 gives linear (1, 0, 0) within 1e-3;
typing `#FFC800` and Enter gives the linear conversion of (255, 200, 0) within 1e-3; `ffc800` is accepted;
`#12` is refused with the colour unchanged; a press outside gives `outside`; a colour made grey keeps the
hue it had on the next frame.
