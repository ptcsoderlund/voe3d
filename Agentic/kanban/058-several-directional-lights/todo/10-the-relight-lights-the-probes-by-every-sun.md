# 10 — The relight lights the probes by every sun
folder: render
after: 09
decisions: 0168, 0357, 0326, 0329, 0350

## Change
Decision 0357 points 1 and 4 in the relight: each further sun lights level 1 of the chain its
bounces name, shadowed by its own layer of the sun map. Read the headers of
`render/src/bounce_relight.c` and `render/shaders/bounce_relight.slang` first.

- `render/src/device_parts.h`: `struct voe_render_relight_record` keeps every member where it is
  and appends `more_count` with three pad words at 2992, then
  `more[VOE_RENDER_DIRECTIONAL_LIGHTS − 1]` of a new `struct voe_render_relight_sun`:
  `{ voe_render_light light; voe_math_float4x4 map; float texel; uint32_t drawn; float strength;
  uint32_t mask; uint32_t bounces; uint32_t pad[3]; }`, 144 bytes. The static_assert gains the new
  offsets and the size; the comment names the further suns.
- `render/src/bounce_relight.c`: the record write fills `more` from the begin's further suns and
  the sun map's layers 1–3, as it fills the first from layer 0. The chains held count each
  further sun at its own bounces, as the first sun's are counted. A sun with no bounces or no
  intensity holds no chain. Header: the record and chain points name every sun.
- `render/shaders/bounce_relight.slang`: the record struct matches. Level 1 of chain n sums, over
  the first sun and each `more` entry whose bounces are n, the same term the first sun takes:
  - by its own map on its own layer (`drawn` false: unshadowed);
  - gated by its own `mask` along its own segment.

  Header: the level 1 point says every sun.
- New `render/tests/bounce_suns.c`, built as `render/tests/bounce_probes_scene.c` builds its scene,
  bounces and reads back (read its header), with its `render/tests/tests.md` entry. Cases:
  1. A sun alone: the reference irradiance at a probe.
  2. The same with an empty `more`: identical.
  3. A dark first sun and the same sun as `more[0]`: the same irradiance within 1%.
  4. A sun and a moon both bouncing once: more than the sun alone.
  5. The moon with bounces 0: the sun alone's picture.
  6. The moon behind a wall, its shadow map drawn on sun 1: the probe behind the wall lit only by
     the sun.

## Done when
`ctest --test-dir build/debug -R "^render/(bounce_suns|bounce_probes_scene|bounce_shadow|blocked_bounce)$"`
passes after the render build.
