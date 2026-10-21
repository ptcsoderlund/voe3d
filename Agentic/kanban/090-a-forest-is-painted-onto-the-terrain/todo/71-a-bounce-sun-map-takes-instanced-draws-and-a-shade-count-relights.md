# 71 — A bounce sun map takes instanced draws, and a shade count relights
folder: render
after: none
decisions: 0168, 0329, 0442, 0450, 0451

## Change
Bug 04, render's half (0451 points 1 and 2): copies may be drawn into a
bounce sun map, and a caller can make a volume relight without recapturing.

- `render/src/draw.c` — `voe_render_frame_draw_instanced` stops asserting in
  a bounce shadow pass; it draws there through the shadow pipeline as in a
  cascade. Point-shadow and capture passes still assert. Its header comment
  follows.
- `render/include/render/device.h` (public; not split) —
  - `voe_render_frame_draw_instanced`'s comment: the bounce shadow pass is
    allowed, why (one view like a cascade, 0450), the two that still assert.
  - `struct voe_render_bounce_frame` gains `uint32_t shade`, last: the
    caller's count of changes to what its sun maps draw that no stale sphere
    marks; a begin whose `shade` differs from the volume's last relit one
    relights it, recapturing no probe; zero, any caller naming none, is the
    old behaviour. The struct's comment and the relight paragraph of
    `voe_render_bounce_shadow_pass_begin` ("no relight is needed") say so.
- `render/src/bounce_probes.h`, `render/src/bounce_probes.c` —
  `voe_render_bounce_lights` gains `uint32_t shade`;
  `voe_render_bounce_probes_lights_changed` compares it, so
  `_relight_needed` is true on a change and `_relit` keeps it. The header's
  "A RELIGHT IS NEEDED" paragraph names it. `_place`'s signature stays.
- `render/src/bounce_volume.c` — the begin, after the place writes this
  update's lights, sets their `shade` from the frame's.
- `render/tests/bounce_probes.c` — a case: placed and relit with shade 1,
  a place with shade 1 again needs no relight; with shade 2 it does, and
  queues no probe.
- `render/tests/bounce_shadow.c` — a case: in the opened sun map, one
  instanced draw of a cube's range over a written instance record moves a
  texel past the clear, as the existing cube draw does.
- `render/src/src.md`, `render/tests/tests.md` — the entries for
  `draw.c`, `bounce_probes.*`, `bounce_volume.c` and the two tests follow.

## Done when
Tests `render/bounce_probes`, `render/bounce_shadow`, `render/instances` and
`render/bounce_capture` pass.
