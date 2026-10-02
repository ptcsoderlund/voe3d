# 01 — Which cube faces a caster reaches, on the CPU
folder: render
after: none
decisions: 0168, 0325

## Change
New pure-CPU module in `render/src/`, beside `light_bins.h` (read its header for the shape such a
module takes here):
- `render/src/point_shadow_faces.h` and `.c`, three functions:
  - a bounding sphere of `count` `voe_render_vertex` positions: centre of their box, radius the
    farthest position from it; returned as a `voe_math_float4` (xyz centre, w radius);
  - that sphere under a world matrix (`voe_math_float4x4`, row-major, column vectors): centre
    transformed, radius times the longest of the three basis columns;
  - the faces a sphere reaches for one light at a position (eye-relative, as the sphere) with a
    range: a 6-bit mask, bit f for face f in the order +X −X +Y −Y +Z −Z (0325 point 1); nought
    when the sphere is past the range; a face's region is its 90° pyramid about the light, each of
    its four side planes moved out by the radius.
- Header points: pure CPU, why the mask (the instanced draw of 0325 point 2), the face order and that
  `shaders/point_shadow.slangh` (a later card) must keep it, a sphere containing the light reaches
  all six.
- `render/src/src.md`: one entry for the pair.
- `render/tests/point_shadow_faces.c`, new, no card: a sphere straight along +X reaches bit 0
  only; one on the +X/+Y diagonal bits 0 and 2; one about the light all six; one beyond range none;
  a cube's 24 vertices give centre 0 and radius √3·half side; a matrix scaling by (1, 3, 1) and
  moving by (5, 0, 0) gives centre (5, 0, 0) and radius times 3.
- `render/tests/tests.md`: an entry.

## Done when
`ctest --test-dir build/debug -R "^render/point_shadow_faces$"` passes.
