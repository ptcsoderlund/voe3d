# 27 — The relight shadows the sun by its own map, never the view's cascades
folder: render
after: 26
decisions: 0168, 0326, 0329

## Change
0329 point 2, the relight's half. The begin's record loses its shadow record: 3d's
`draw_bounce.c` stops setting it in card 28, not here. Read `bounce_relight.c`'s and
`bounce_shadow.c`'s headers first.
- `render/include/render/device.h`: `struct voe_render_bounce_frame` loses `shadow`;
  `voe_render_bounce_begin`'s and `voe_render_bounce_relight`'s comments: the sun is shadowed by
  this frame's bounce shadow map when one was drawn, lit outside its box, unshadowed when none was
  (a sun that does not cast); the cascades never reach the relight (0329).
- `render/src/device_parts.h`: the device's copy of the begun frame to match.
- `render/src/bounce_relight.c`: binding 9 is this slot's bounce shadow map through the shadow
  sampler, in place of the cascade array; `struct relight_record` holds, in place of the shadow
  record, the map's view × projection, its texel (the box's width, from the projection, over
  `VOE_RENDER_BOUNCE_SHADOW_TEXELS`) and whether it was drawn this frame; its static asserts to
  match. Header: the set layout's 9 and the record.
- `render/shaders/bounce_relight.slang`: the record struct matches; the sun's shadow at a surface
  point is one compare in that map, pushed a texel along n, 1 outside its box or when not drawn;
  the cascade loop goes. Header: level 1's sun shadow is the volume's own map.
- `render/tests/bounce_probes_scene.c`: each frame that relights, after the capture passes, the test
  opens the bounce shadow pass with a light view looking down the sun at the volume's centre, half
  60 m, and draws the scene into it when opened, in place of the record's `shadow`; the camera pass
  keeps its cascade. Every claim passes as before.
- `render/tests/bounce_settle.c`, `bounce_read.c`, `bounce_volume.c`, `bounce_capture.c`,
  `bounce_shadow.c`: only where they name the field.
- `render/shaders/shaders.md`, `render/src/src.md`, `render/tests/tests.md`: entries that say the
  relight reads the cascades.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes and
`! grep -n "cascades\[" render/shaders/bounce_relight.slang` exits 0.
