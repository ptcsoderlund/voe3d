# 35 — Render no longer ties `unshaded` to a scene without light
folder: render
after: none
decisions: 0168, 0287, 0290

## Change
Bug 04. `3d` stops setting `unshaded` for a scene with no light (0290 point 1); `render` keeps the
flag, set by no folder today (0290 point 3). Only comments change: no code, shader logic, layout
or test body.

- `render/include/render/device.h`, the `voe_render_light` comment (around "`unshaded` NON-ZERO
  MEANS THIS PASS HAS NO SUN"): drop "the owner of the scene's light, `3d`, sets it when a scene
  has none (ADR-0238)". Points: a pass may ask for base colours with it; no folder sets it today
  (0290); a scene with no light is the zeroed light, drawn black on lit surfaces (0287).
- `render/shaders/draw.slang`, the comment near line 109 that cites ADR-0238: the same points.
- `render/tests/unshaded.c` header: its reason is no longer "what a scene with no light looks
  like (ADR-0238)"; say it proves the flag render still offers, and that the zeroed light is black
  (its case 2), which is what a scene with no light now draws (0287).
- `render/tests/tests.md`, the `unshaded.c` entry: only if it cites 0238, the same change.

## Done when
`grep -rln 0238 render/include render/src render/shaders render/tests` prints nothing (exit 1).
