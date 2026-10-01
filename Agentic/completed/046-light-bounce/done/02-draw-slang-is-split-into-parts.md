# 02 — draw.slang is split into parts
folder: render
after: 01
decisions: 0168, 0308

## Change
0308 point 8; a move, no behaviour changes. `render/shaders/draw.slang` is
864 lines. Read it, `render/shaders/shaders.md` and the header comment of
`render/src/pipeline.c` (how the module is embedded).

- `render/shaders/lighting.slangh`, new: the sun's direct light, the shadow
  lookup over the cascades and the fill weight and fill (0273, 0275, 0276),
  with what they need of the camera block's types; header comment.
- `render/shaders/water.slangh`, new: the water shading (waves, fresnel,
  thickness over the depth copy, 0305); header comment.
- `render/shaders/draw.slang`: keeps the blocks, bindings, the vertex stage
  and the entry points, and includes both parts. Where a part needs a
  binding, the binding stays in `draw.slang` above the include, or moves to a
  third part `bindings.slangh` included first, whichever leaves no file over
  ~500 lines and no cycle.
- `render/shaders/shaders.md`: an entry for each new part; the
  `draw.slang` entry says what it still holds.

## Done when
After the folder's build, the tests `render/shadow`, `render/water`,
`render/offscreen`, `render/unshaded`, `render/surface_maps` and
`render/depth_copy` still pass, and `wc -l render/shaders/*.slang*` shows no
file over 600 lines.
