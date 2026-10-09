# 03 — A terrain record draws the shared grid at its heights, morphed by distance
folder: render
after: 02
decisions: 0168, 0250, 0396

## Change
0396 point 3, the vertex stage. Read the headers of `render/shaders/draw.slang` and
`render/shaders/bindings.slangh` first.

- `render/include/render/device.h`, `voe_render_object`:
  - `reserved[0]` becomes `uint32_t heights`: a heights texture's index plus one; nought draws the
    geometry as ever. `reserved` keeps two words.
  - New `voe_math_float4 terrain`: the node's corner x and z in the landscape's own metres (its
    grid centred on the origin, 0379 point 1), the node's side, the landscape's side.
  - New `voe_math_float4 morph`: the morph's start and end in metres from the eye, the node box's
    bottom y and its height (above nought).
  - Header points: the geometry is a grid of x, z in 0..1 whose y is ignored; `world` maps the node
    box to camera-relative space; `normal` is the landscape's own normal matrix, not the box's.
- `render/src/records_layout.c`, `render/src/descriptors.c`: the asserted sizes and offsets.
- `render/shaders/bindings.slangh`: the mirrored struct.
- New `render/shaders/terrain.slangh` (entry in `render/shaders/shaders.md`): one function taking
  the object and the vertex, giving local position, landscape normal and uv. Points: grid units
  32 a side; morph factor from the unmorphed point's distance to the eye (the origin); odd grid
  lines slide to the even ones by it; landscape xz clamped to ± side/2; height from four `Load`s
  blended by hand (R32F filtering is not guaranteed); local y from box bottom and height; normal a
  central difference at the node's step in texels; uv the landscape's 0..1.
- `render/shaders/draw.slang`: every vertex entry that takes a mesh (solid, blended, shadow,
  point shadow, capture) calls it when `heights` is not nought.
- New headless test `render/tests/terrain_vertex.c` (entry in `render/tests/tests.md`): a 2 × 2
  quad grid geometry, a 3 × 3 heights texture all 2 m written with `_write_heights`, a camera
  looking level at y = 1: drawn as terrain the ground covers the target's pixels above the
  middle row as a 2 m plane would; the same record with `heights` nought does not.

## Done when
`render/tests/terrain_vertex.c` passes; the render library builds, so `records_layout.c`'s
build-time asserts hold.
