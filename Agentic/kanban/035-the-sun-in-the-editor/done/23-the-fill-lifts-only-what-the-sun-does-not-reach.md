# 23 — The fill lifts only what the sun does not reach
folder: render
decisions: 0168, 0273, 0275, 0276

## Change
Bug 02: the fill is added flat to every lit surface, so it tints the sunlit ones. The fault
is in the shader alone; `render`'s record and every caller stay as they are.

- `render/shaders/draw.slang`: in the lit path, the fill term (near "The fill, from no
  direction and outside the shadow") is weighted by 0276: reach = saturate(N·L) × the shadow
  factor already computed for the sun's direct term (1 when no cascade covers the surface or
  shadow `count` is nought), weight = 1 − smoothstep(0, 0.3, reach), term = weight × fill ×
  base colour. The 0.3 is one named constant. Rewrite the header's "ONE DIRECTIONAL LIGHT AND
  ONE FLAT FILL" and the last sentence of the cascade paragraph: the fill lifts only the shade
  (0275), whole where the sun does not reach, gone from a reach of 0.3 (0276); a shadowed
  surface still reads its fill and nothing more.
- `render/include/render/device.h`, the `fill` paragraph of `voe_render_light`'s comment:
  added only where the sun does not reach, fading out as it does (0275, 0276), not to every
  lit surface. The layout does not change.
- `render/tests/unshaded.c`: keep cases 4 and 5; add two, the shaded cube with fill
  (0.25, 0.25, 0.25) against the same picture with fill zero: (6) the sun pointing straight at
  the face read, and (7) the sun meeting that face at N·L of 0.5 — each reads the same with
  and without fill within the file's tolerance. List them in the file's header and on its
  line in `render/tests/tests.md`.
- `render/shaders/shaders.md` and `render/include/render/render.md`: only if either says the
  fill is flat or on every surface (a grep for "fill" shows).

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render
voe_test_render_unshaded && ctest --test-dir build/debug -R "^render/"` exits 0.
