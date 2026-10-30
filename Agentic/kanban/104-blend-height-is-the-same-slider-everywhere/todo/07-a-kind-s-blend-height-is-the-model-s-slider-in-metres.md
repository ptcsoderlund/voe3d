# 07 — A kind's blend height is the model's slider, in metres
folder: editor
after: 06
decisions: 0168, 0481, 0494, 0498

## Change
A foliage kind's Blend height row on the Inspector's Foliage section looks and works as a model's
(card 06): label "Blend height", the slider over 0.05–10 m, the figure in metres. The look and the
landscape file keep whole centimetres (0498 point 4); only the row converts.

- `editor/src/inspector_foliage.c`:
  - The kind's blend draw: label "Blend height" (no "cm"); slider value the look's height (0 still
    meaning `VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_DEFAULT`) / 100, ends `VOE_3D_MODEL_BLEND_HEIGHT_LEAST`
    and `VOE_3D_MODEL_BLEND_HEIGHT_GREATEST` (3d/model_component.h, card 05); its figure into
    `blend_figures[i]` through `figure` (inspector_value.h) with unit `m`.
  - The kind's blend read: `voe_ui_slider_action` over those same ends; on a change the metres
    become round(metres × 100), clamped to `VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN`–`_MAX`, and ask
    the look with that height as the row does now, still one undo step when it lets go.
  - Its top comment says metres, not "cm".
- `editor/src/inspector_foliage.h`: the header says the Blend height slider is in metres, the
  model's slider and range, stored as whole centimetres.
- `editor/src/sculpt.h`: the comment over `blend_sliders` / `blend_figures` says the slider shows
  metres over the model's range and the look keeps whole centimetres; check
  `VOE_EDITOR_SCULPT_FIGURE` still holds "10.00 m" and its terminator.
- `editor/src/src.md`: the `inspector_foliage.h` entry says the Blend height is in metres.

## Done when
`! grep -q 'Blend height cm' editor/src/inspector_foliage.c` and
`grep -q VOE_3D_MODEL_BLEND_HEIGHT_GREATEST editor/src/inspector_foliage.c` exit 0, and the editor
builds.

The human's, the feature's How to test:
1. On the hill, a model with Blend with ground on: Blend height a slider, e.g. 0.50 m; it stops at
   0.05 m and 10.00 m and the blend follows the drag.
2. The landscape's Foliage section, a kind's row: the same slider in metres, same ends; a kind that
   blended at 50 cm shows 0.50 m.
3. Both at 1.20 m: a model and a tree beside it blend up to the same height.
4. Undo each change: value and look go back.
5. Save, close, reopen: both as left. A project saved before this: its blend heights look as they
   did.
6. A light, the clouds row and a fog volume: a value with a natural range is a slider stopping at
   its ends; a position is still a number box.
