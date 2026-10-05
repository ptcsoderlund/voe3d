# 08 — A bounce begin keeps every sun
folder: render
after: 07
decisions: 0168, 0357, 0326, 0350

## Change
The bookkeeping half of decision 0357 points 1 and 4: a bounce begin takes the further suns, and
a change to any of them relights. The relight does not light by them yet (card 10).

- `render/include/render/device.h`: `struct voe_render_bounce_frame` gains
  `voe_render_directional_lights more;` last. Each entry's `light`, `bounces`, `bounce_strength`
  and `blockers` (its place mask) are read; its `shadow` is not. Comment points:
  - the further suns bounce as the first one does;
  - the begin asserts on their bounces past `VOE_RENDER_BOUNCES_MAX`, a count past 3, and a mask
    bit past the blocker count.
- `render/src/bounce_probes.h`: `voe_render_bounce_lights` gains `more_count` and
  `more[VOE_RENDER_DIRECTIONAL_LIGHTS − 1]` of `{ light, bounces, strength, mask }`. Its unused
  entries are zero. `voe_render_bounce_probes_place` takes `const voe_render_directional_lights
  *more` after `sun_strength`, and the usage example and the "relight is needed" point name them.
- `render/src/bounce_probes.c`: place copies them. `relight_needed` compares the count, and each
  sun's light, bounces, strength and mask as it does the first's. A light with no bounces or no
  intensity counts as none, as the first does.
- `render/src/bounce_volume.c`: the begin asserts as above and passes `&frame->more` to place.
  `device->bounce_frame` keeps the further suns' masks next to `blockers.sun`, wherever
  the begin stores the first's (read the begin's code near the place call).
- `render/src/src.md`: the `bounce_probes.h` entry says every sun.
- `render/tests/bounce_probes.c`: every place call passes an empty `more`. New cases, written as
  that file's lamp cases are:
  - the same moon again needs no relight;
  - a moon's intensity, its mask or its bounces changed relights;
  - a moon added, or taken away, relights.

  Its header lists them.

## Done when
`ctest --test-dir build/debug -R "^render/(bounce_probes|bounce_volume|bounce_probes_scene)$"`
passes after the render build.
