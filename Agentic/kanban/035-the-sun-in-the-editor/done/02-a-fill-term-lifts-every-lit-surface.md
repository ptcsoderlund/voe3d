# 02 — A fill term lifts every lit surface
folder: render
decisions: 0168, 0273, 0258

## Change
Additive to the public surface: a zeroed field keeps every caller as it is.

- `render/include/render/device.h`, `voe_render_light`: add `voe_math_float3 fill` and a
  `float reserved` after `unshaded`, keeping the sixteen-byte padding. Rewrite the record's
  comment: what fill is (linear, colour already times strength, added to lit surfaces from no
  direction and never shadowed), zero is none, `unshaded` reads none. Fix the paragraph that
  says nothing lights the side facing away.
- `render/shaders/draw.slang`: the struct gains the same two fields; the lit path adds
  `fill × base colour` (after the base colour texture, before emission if any) outside the
  shadow factor. Rewrite the lighting header's "no ambient" paragraphs to say the fill is the
  one term that is not the sun (0273).
- `render/src/device_parts.h` and `render/src/descriptors.c`: whatever mirrors the light's size
  and offsets, so the asserts still hold.
- `render/tests/unshaded.c`: add a case: the cube lit by a sun pointing away with fill
  (0.25, 0.25, 0.25) reads a quarter of its base colour (within the file's tolerance), and with
  fill zero still reads black. Name the case in `render/tests/tests.md`.
- `render/include/render/render.md` and `render/shaders/shaders.md` only where they say "no
  ambient".

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render voe_test_render_unshaded
&& ctest --test-dir build/debug -R "^render/"` exits 0.
