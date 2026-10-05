# 09 — The relight sun map holds a layer per sun
folder: render
after: 08
decisions: 0168, 0357, 0329, 0330

## Change
Decision 0357 point 4: the map and its pass. The relight still reads layer 0 only (card 10).
`3d/src/draw_bounce.c` will not build until card 11 passes the new argument. Do not touch `3d`.

- `render/include/render/device.h`: `voe_render_bounce_shadow_pass_begin(device, uint32_t sun,
  const voe_render_view *light, bool *opened)`. Sun 0 is the begin's first sun, and sun i its
  `more[i − 1]`. `opened` is false, with true returned, when that sun has no bounces, no
  intensity or is unshaded, or when the begin will not relight. Comment points:
  - once per sun per begin;
  - a sun at or past `1 + more.count`, or opening one sun twice, asserts.
- `render/src/device_parts.h`: `struct voe_render_bounce_shadow` holds a
  `VOE_RENDER_DIRECTIONAL_LIGHTS`-layer image, a 2D-array view of every layer for the relight,
  one attachment view per layer, `light[4]` and `drawn[4]`. Comment updated (16 MiB a slot).
- `render/src/device_internal.h`: beside `pass_bounce_shadow`, the open pass's sun.
- `render/src/bounce_shadow.c`: startup makes and settles the layers and views, and close frees
  them. The begin draws onto the sun's layer, and the end moves only that layer back. The header's
  map and cost points are updated.
- `render/src/bounce_volume.c`: the begin clears all four `drawn`.
- `render/src/bounce_relight.c`: binding 9's write takes the array view. The record's `sun_map`
  and `sun_drawn` come from layer 0. Binding 9's header line is updated.
- `render/shaders/bounce_relight.slang`: `relight_sun_shadow` becomes a `Sampler2DArrayShadow`,
  and its one lookup reads layer 0.
- Every caller in `render/tests` passes sun 0: `blocked_bounce.c`, `blocker_kinds_bounce.c`,
  `bounce_probes_scene.c`, `bounce_shadow.c`.
- `render/tests/bounce_shadow.c`: new cases, added to its header:
  - a begin with a moon in `more` opens sun 0 and sun 1, each drawn once;
  - reading layer 1 back holds the moon's depth while layer 0 holds the sun's;
  - with the moon's bounces at 0, its open gives `opened` false.
- `render/src/src.md`: the `bounce_shadow.c` entry says a layer per sun.

## Done when
`ctest --test-dir build/debug -R
"^render/(bounce_shadow|blocked_bounce|blocker_kinds_bounce|bounce_probes_scene)$"` passes after
the render build.
