# 14 — 046's bounce map, pass and pipeline are removed
folder: render
after: 13
decisions: 0168, 0317, 0326

## Change
0326 point 9, second half; nothing opens a bounce pass since card 11.
- `render/include/render/device.h`: `voe_render_bounce_pass_begin` and
  `VOE_RENDER_BOUNCE_TEXELS` go; every comment that names a bounce pass or map changes: the
  file header's order of passes, `passes` (a frame spends one per cascade, one per capture pass
  and one per camera pass), `voe_render_pass_begin`'s "except an open bounce pass",
  `_pass_end`, `_frame_draw`, `_draw_blended`, `_clear_depth`, `_copy_depth`, `_frame_end`.
- `render/src/bounce_map.c`: deleted.
- `render/src/pass.c`: the bounce pass begin, its block, and every `pass_bounce` branch, the
  close of an open bounce pass by the next pass included; header paragraph.
- `render/src/draw.c`, `render/src/element.c`, `render/src/frame.c`,
  `render/src/point_shadow.c`: their `pass_bounce` branches and asserts; `frame.c`'s close of a
  bounce pass at the frame's end.
- `render/src/pipeline.c`: the bounce pipeline and its formats; the count in the header.
- `render/shaders/draw.slang`: `voe_render_draw_bounce` and its output struct; header phrase.
- `render/src/device_parts.h`, `render/src/device_internal.h`, `render/src/device.c`: the map,
  `bounce_view`, `bounce_sun`, `bounced`, `pass_bounce`, `pipeline_bounce`, their startup and
  shutdown, and the calls declared for them.
- `render/tests/bounce_map.c`: deleted. Any other render test opening a bounce pass loses it.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`,
  `render/render.md`: entries.

## Done when
`! grep -rn "pass_bounce\|bounce_map\|VOE_RENDER_BOUNCE_TEXELS\|voe_render_bounce_pass_begin" render --include=*.c --include=*.h --include=*.slang*`
exits 0.
