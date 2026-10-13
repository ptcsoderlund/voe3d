# 02 — A pass exposes and tone maps its mesh draws
folder: render
after: none
decisions: 0168, 0405

## Change
The record of 0405 point 2 and points 7 and 8; the sky itself, its light and haze are later cards and
read none of it yet. Read the headers of the files below and `render/tests/unshaded.c` as the pattern.

- `render/include/render/device.h`: `voe_render_sky { uint32_t drawn; float light; float haze; float
  exposure; uint32_t tone; uint32_t reserved[3]; }`, static_assert 32 bytes; `VOE_RENDER_TONE_CLIP` 0,
  `VOE_RENDER_TONE_SOFT` 1; `voe_render_pass_camera` gains `voe_render_sky sky` after `more`. Comment
  points: zeroed is the old picture byte for byte; each field (0405 point 2), `drawn`, `light` and
  `haze` read by later cards; exposure in stops then the curve at every mesh exit; draws after the
  pass's first depth clear are drawn as authored; elements never. Rewrite the "no exposure and no tone
  mapping yet" and "NOTHING IS TONE MAPPED" comments of `voe_render_light` to say the sky record now
  owns both. The `voe_render_pass_begin` comment names the asserts below.
- `render/src/device_parts.h`: `struct voe_render_frame_block` gains the record, last, padded as the
  block's other members; `render/shaders/bindings.slangh` its twin, and `voe_render_draw_push` a fifth
  word `plain`. `render/src/records_layout.c` / `render/src/descriptors.c`: their size and offset
  checks cover the new member.
- `render/src/pass.c`: copies `camera->sky` into the block (zeroed for a pass with no camera and in
  shadow, point-shadow and capture passes); asserts a number not finite, light or haze below nought,
  exposure past ±16, tone past SOFT.
- `render/src/draw.c`: the pass remembers its first `voe_render_frame_clear_depth`; every mesh draw
  after it pushes `plain` 1, every other 0; the push grows to five words (the shared range is already
  64 bytes, `pipeline.c` unchanged). Header names the word.
- `render/shaders/tone.slangh` (new part, header comment): `float3 voe_render_expose(float3 linear)`:
  × 2^exposure, then Clip as is or Soft by Khronos PBR Neutral; returns its input when `plain`.
- `render/shaders/draw.slang`: the lit and unlit exits pass their colour through it; a blended exit
  on its straight colour, alpha kept. `render/shaders/water.slangh`: its exit likewise.
- `render/tests/exposure.c` (new, headless): an unlit quad of 0.5 at a zeroed sky against exposure 0
  Clip, the same byte; exposure 1 reads 255, −1 reads the sRGB of 0.25; Soft reads the sRGB of PBR
  Neutral(0.5) worked out in the test, within 2/255; a lit quad under a sun of 8 reads 255 at Clip and
  below 255 but above its Soft picture at sun 1 at Soft; a quad drawn after a depth clear at exposure
  −2 reads as authored; a blended quad at exposure 1 reads its alpha-blended doubled colour.
- `render/render.md`, `render/include/render/render.md`, `render/src/src.md`,
  `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed where what they say changed,
  one new each for `tone.slangh` and `exposure.c`. Each at most 300 characters.

## Done when
The tests `render/exposure`, `render/unshaded`, `render/water` and `render/directional_lights` pass after
the folder's build.
