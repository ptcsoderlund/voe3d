# 47 — Occlusion darkens only the fill and the bounce
folder: render
after: none
decisions: 0168, 0400

## Change
0400: an occlusion map's red darkens only the light that does not come straight from a light. Today
the fragment stage multiplies the direct term (directional lights and point lights) by it and leaves
the indirect term whole; turn that round. This one change serves a `.material`'s ORM map and a glTF
model's occlusion texture alike, both arriving as the record's `occlusion_texture`.

- `render/shaders/draw.slang` — in the lit exit of the mesh fragment stage, where `lit` is summed:
  the direct term (`voe_render_directionals` plus `voe_render_points`) is no longer multiplied by
  `occlusion`; the result of `voe_render_indirect` (the bounce with the fills as its floor) is.
  Emission stays unoccluded. Rewrite the header paragraph "OCCLUSION MULTIPLIES THE LIGHT, WHICH IS
  NOT WHERE glTF PUTS IT": occlusion now multiplies the fill and the bounce, as glTF specifies; never
  the sun's direct light, a lamp's, or shadows, because baked occlusion on direct light doubles the
  shadows (0400). Fix the nearby comment above the sum if it says otherwise. Line count unchanged.
- `render/shaders/lighting.slangh` — only if its header says where occlusion is applied; it
  must agree.
- `render/shaders/shaders.md` — `draw.slang`'s line gains that occlusion darkens the indirect term
  only.

Test: add cases to `render/tests/surface_maps.c` (read its header; `render/include/render/device.h`'s
`voe_render_light` for the fill), each with a one-pixel DATA occlusion texture:
- the reference's sun from +Z with no fill, an occlusion texel of red 0: the same as case 1, the sun's
  direct light undarkened;
- the sun pointing away and a fill turned up, occlusion red 255: lit by the fill alone;
- the same with occlusion red 0: darker than the previous case in every channel.
Update the header's case list, `CASES` and the capacities.

## Done when
`ctest --test-dir build/debug -R '^render/surface_maps$'` passes with the new cases.
