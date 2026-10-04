# 20 — A bounce begin carries the kinds and relights on a change
folder: render
after: 19
decisions: 0168, 0332, 0350

## Change
The bounce's C side of 0350 points 2 and 5: the relight record holds the kinds and the sun's mask,
and a change relights; no shader reads them yet (card 22). Read the headers of
`render/src/bounce_probes.h`, `render/src/bounce_probes.c`, `render/src/bounce_volume.c`,
`render/src/bounce_relight.c`, `render/src/device_parts.h`, and `struct voe_render_bounce_frame` and
`voe_render_light_blockers` in `render/include/render/device.h`.

- `render/include/render/device.h`: `struct voe_render_bounce_frame`'s `blockers` comment states
  0350 point 5 in place of 0347 point 4's lines, and that a change of kinds or the sun's mask
  relights; begin asserts on the three words as pass begin does.
- `render/src/bounce_probes.h`, `render/src/bounce_probes.c`: `voe_render_bounce_lights` gains
  `walls`, `indoors`, `sun`; `_place` copies them; `_relight_needed` also answers true when any of
  the three differs. Header points.
- `render/src/bounce_volume.c`: begin's asserts on the three words.
- `render/src/device_parts.h`: `struct voe_render_relight_record` gains the three (reserved words
  if free, else after the blockers), offset and size asserts moved to match; header point.
- `render/src/bounce_relight.c`: the record is written with the begun three.
- `render/shaders/bounce_relight.slang`: the record's layout matches; nothing reads them.
- `render/tests/bounce_probes.c`: a blocker turned from Room to Wall relights; the same kinds
  again do not; a sun mask changed from 0 to 1 relights.
- `render/src/src.md`, `render/tests/tests.md`: the entries for what changed. Each at most 300
  characters.

## Done when
The test `render/bounce_probes` passes, and `render/bounce_volume` and `render/blocked_bounce`
still pass, after the folder's build.
