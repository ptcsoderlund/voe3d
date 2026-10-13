# 29 — A mirrored normal map tilts mirrored
folder: render
after: 28
decisions: 0168, 0278, 0421

## Change
0421 point 4: pin the derivative frame's handedness, and fix `voe_render_mapped_normal` in
`render/shaders/draw.slang` only if the test shows it wrong.

- `render/tests/surface_maps.c`: a case of two quads side by side facing the camera, the second with its
  u running the other way across the world (a mirrored half), both wearing one normal map tilted towards
  +u (red high, green half, blue high) and a sun low from world +x. The first quad's pixels are brighter
  than a flat-mapped quad's, the second's darker: the bump turns over with its picture, as Blender's
  does. The same along v: a map with green high and a sun low from world +y, on a quad whose v runs
  down the world and on one whose v runs up: the first brighter than flat, the second darker.
- If it fails: correct the frame's sign in `voe_render_mapped_normal` and say in its comment what was
  wrong. If it passes, the comment's "Seams where a frame flips" sentence stays and gains that mirrored
  halves are right by the determinant's sign, and that this test says so.
- `render/tests/tests.md`: the `surface_maps.c` entry grows by the mirrored case (at most 300
  characters).

## Done when
The test `render/surface_maps` passes after the folder's build.
