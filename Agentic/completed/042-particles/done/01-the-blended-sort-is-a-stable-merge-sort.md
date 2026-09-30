# 01 — The blended sort is a stable merge sort
folder: 3d
after: none
decisions: 0168, 0298

## Change
0298 point 7. Particles put thousands of blended objects into one pass,
rebuilt unsorted each frame, and the insertion sort is quadratic on that.

- `3d/include/3d/depth_sort.h`: `voe_3d_depth_sort(const float *view_z,
  uint32_t count, uint32_t *order, uint32_t *scratch)`. `scratch` is the
  caller's, `count` elements. The header says why a merge sort (particles,
  0298), that it stays stable (equal depths keep their order), and that it
  still allocates nothing.
- `3d/src/depth_sort.c`: a bottom-up merge sort over `order`, through
  `scratch`, comparing strictly `<` on z so ties keep their order. Its header
  comment says the same.
- `3d/src/draw_group.c` (read `3d/src/draw_group.h` first): the one caller
  passes scratch from the arena it already takes room from, sized by the
  group's count.
- `3d/tests/depth_sort.c`: the existing cases take the new argument; a new
  case sorts 4096 depths from a fixed seed and checks that they come out
  ascending, and that equal depths keep their input order.
- `3d/src/src.md`: the `depth_sort.c` entry says merge sort.

## Done when
`ctest --test-dir build/debug -R '^3d/(depth_sort|draw_system)$'` passes,
after the folder's build.
