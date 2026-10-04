# 06 — A bounce begin carries its blockers
folder: render
after: 03
decisions: 0168, 0332, 0347

## Change
The bounce is handed the blockers and relights when they change (0347 point 4); nothing reads them
in a shader yet (card 07). Read the headers of `render/src/bounce_probes.h`,
`render/src/bounce_probes.c`, `render/src/bounce_volume.c`, `render/src/bounce_relight.c`, and
`struct voe_render_bounce_frame` and `voe_render_light_blocker` in
`render/include/render/device.h`.

- `render/include/render/device.h`: `struct voe_render_bounce_frame` gains `voe_render_light_blockers
  blockers` last, about the eye as `points` are; comment: copied at begin, what the relight does with
  them (0347 point 4), that a change relights and an eye that moves does not; begin asserts as the
  pass camera does on them.
- `render/src/bounce_probes.h`, `render/src/bounce_probes.c`: `voe_render_bounce_lights` gains
  `uint32_t blocker_count` and `voe_render_light_blocker blockers[VOE_RENDER_LIGHT_BLOCKERS]`;
  `voe_render_bounce_probes_place` gains `const voe_render_light_blockers *blockers` after
  `point_lights` and copies them in; `_relight_needed` also answers true when the count differs or a
  blocker differs: rows' xyz and the sphere's radius beyond 1e-4, or its sphere centre about the
  corner beyond a millimetre, or row.w + row.xyz · corner beyond 1e-4 (both eye-invariant, as lamps
  are compared about the corner). Header points on both.
- `render/src/bounce_volume.c`: begin passes `&frame->blockers` to `_place`; its asserts.
- `render/src/device_parts.h`: `struct voe_render_relight_record` gains the blocker count (a reserved
  word if one is free) and the blockers after the lamps, its offset and size asserts moved to match;
  header point.
- `render/src/bounce_relight.c`: the record is written with the begun blockers; nothing else.
- `render/shaders/bounce_relight.slang`: the record's layout matches; nothing reads the new fields.
- `render/tests/bounce_probes.c`: the existing `_place` calls pass none; new cases: a blocker added
  relights, the same blockers again do not, the same blockers with the eye and corner both moved by
  3 m do not, a blocker moved 1 cm does.
- `render/src/src.md`, `render/tests/tests.md`: the entries for what moved. Each at most 300
  characters.

## Done when
The test `render/bounce_probes` passes, and `render/bounce_volume`, `render/bounce_settle` and
`render/bounce_probes_scene` still pass, after the folder's build.
