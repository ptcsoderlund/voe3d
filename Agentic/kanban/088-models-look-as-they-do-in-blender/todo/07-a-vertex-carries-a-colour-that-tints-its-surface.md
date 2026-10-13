# 07 — A vertex carries a colour that tints its surface
folder: render
after: none
decisions: 0168, 0421

## Change
0421 point 1. Every vertex builder outside `render` keeps compiling unchanged and draws as before,
because a zeroed colour reads white.

- `render/include/render/device.h`: `voe_render_vertex` gains `uint16_t colour_complement[4]`, after
  `uv`. Its comment: read as 1 − value/65535 per channel, rgba, linear; why the complement (a zeroed
  vertex is opaque white, so builders that never heard of it are unchanged); that it multiplies the
  record's base colour, alpha included. The struct's opening comment mentions the fourth attribute.
- `render/src/pipeline.c`: the vertex input gains location 3, R16G16B16A16_UNORM, at the field's
  offset; the attribute count follows. Any pipeline that reads `voe_render_vertex` takes it.
- `render/shaders/draw.slang`: `voe_render_draw_vertex_in` gains the attribute; the vertex stage hands
  the colour (1 − complement) to the fragment stage; the fragment multiplies `base` by it right after
  `base` is formed, before the alpha mode, so the lit, unlit and water exits and the cut all see it.
  The capture pass's vertex and fragment stages multiply its base colour the same way, so the bounce
  sees the tint. The terrain's path reads a white colour. The header gains one paragraph on it.
- `render/shaders/shaders.md`: the `draw.slang` entry mentions vertex colour (at most 300 characters).
- `render/tests/vertex_colours.c` (new, headless, read the pattern of `render/tests/surface_maps.c`):
  an unlit white record on a quad whose vertices carry red reads red; the same quad with zeroed colours
  reads white; a lit quad with half-grey vertex colour reads about half the brightness of white ones.
- `render/tests/tests.md`: an entry for it (at most 300 characters).

## Done when
The test `render/vertex_colours` passes after the folder's build, with the folder's other tests.
