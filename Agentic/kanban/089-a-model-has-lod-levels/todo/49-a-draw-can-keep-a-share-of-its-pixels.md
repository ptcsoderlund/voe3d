# 49 — A draw can keep a share of its pixels
folder: render
after: none
decisions: 0168, 0432

## Change
0432 point 5: the object record gains `dither`, so `3d` can draw two levels of a model each on
its complementary share of the pixels. Nothing above `render` sets it yet (card 53).

- `render/include/render/device.h`, `voe_render_object` (line ~777): after `layers`, `float dither`
  and three `uint32_t` reserved words, as the shading record pads. The struct's comment gains a
  paragraph: 0 draws whole, so a zeroed record is unchanged; d in (0, 1] keeps the pixels whose
  threshold is under d, d in [−1, 0) those at or above −d, so d and −d drawn at one place cover
  each pixel exactly once; the threshold is a 4 × 4 ordered pattern on the screen; read by the
  mesh fragment stage in every pipeline, not by shadow, point-shadow or capture passes; why
  (0432: a model's two levels dissolve across a band); the record grew from 256 to 272 bytes.
- `render/shaders/bindings.slangh`: the shader's `voe_render_object` gains the same four members,
  three scalars and not a vector, as its padding paragraph says; the sizes paragraph above it ends
  at 272.
- `render/shaders/draw.slang`, `voe_render_draw_fragment` (line ~389): a small function beside
  `voe_render_median` giving the pixel's threshold from `SV_Position.xy` (a 4 × 4 Bayer matrix,
  (index + 0.5) ÷ 16), and a discard of a pixel the object's `dither` drops, placed beside the
  cut-out's discard (line ~531) for the reason its comment gives. The header's paragraph on the
  fragment stage names the dither in a phrase.
- `render/src/records_layout.c`: the size assert 272; an `offsetof(voe_render_object, dither) ==
  256` assert beside the others.
- `render/tests/dither.c` (new), headless, set up as `render/tests/cutout_far.c` is (device,
  camera pass, unlit quad filling the picture, read back): an unlit red quad filling the
  picture:
  1. `dither` 0: every pixel red;
  2. `dither` 0.5: red pixels within 1 % of half the picture, the rest the clear colour;
  3. red at 0.5, then a green quad at the same place at −0.5: no pixel the clear colour, red and
     green each within 1 % of half;
  4. red at 0.25: red within 1 % of a quarter.
  Its header says what it shows.
- `render/tests/tests.md`: the entry for `dither.c`. `render/shaders/shaders.md`: the
  `bindings.slangh` and `draw.slang` entries name the dither in a phrase.

## Done when
The test `render/dither` passes after the folder's build, and `render/offscreen` and
`render/cutout_far` still pass.
