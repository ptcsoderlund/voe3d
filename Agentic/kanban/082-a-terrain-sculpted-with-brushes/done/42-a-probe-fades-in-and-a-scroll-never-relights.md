# 42 — A probe fades in, and a scroll never relights
folder: render
after: 41
decisions: 0168, 0387, 0389

## Change
The probes' bookkeeping (0389 points 4, 5 and 7), on the CPU only.

- `render/src/bounce_probes.h`:
  - `VOE_RENDER_BOUNCE_FADE` 16: places a new picture takes to reach full weight.
  - `VOE_RENDER_BOUNCE_UNSEEN_RADII` 16: past this many of its radii, a caster is under one face texel.
  - In `voe_render_bounce_probes`: `uint8_t ready[VOE_RENDER_BOUNCE_PROBES_TOTAL]` and a `fading` bitset.
    `relit_corner` becomes the relit world origin.
  - New `bool voe_render_bounce_probes_lights_changed(const voe_render_bounce_probes *p, const
    voe_render_bounce_lights *lights)`: true when the lights differ from the last relit.
  - New `bool voe_render_bounce_probes_fading(const voe_render_bounce_probes *p)`: true when any probe is
    fading.
  - Header points to update: a stale sphere's reach; the world-origin compare; readiness.
- `render/src/bounce_probes.c`:
  - **place**: a stale sphere queues the probes whose centre is within w + min(VOE_RENDER_BOUNCE_REACH ×
    spacing / VOE_RENDER_BOUNCE_SPACING, UNSEEN_RADII × w). A probe brought in gets ready 0 and is not
    fading. Every fading probe gains 1 of readiness, and stops fading at FADE.
  - **take**: a probe that held no picture starts at ready 0, fading. One that held a picture keeps its
    readiness.
  - **relight_needed**: a changed probe, a fading probe, or `_lights_changed`.
  - **lights compare**: lamps and blockers are compared about the world origin, corner − cell × spacing,
    in place of the corner, with the same tolerances.
  - **relit**: keeps that world origin.
- `render/include/render/device.h`: in `struct voe_render_bounce_frame`'s comment, `stale` is spheres of a
  caster's own radius and render adds the reach. Blockers and lamps are compared about the world origin, so
  a grid that scrolls does not relight. Update the stale-radius wording at `voe_render_bounce_begin` to
  match. No signature changes.
- `render/tests/bounce_probes.c`: fix any case the new reach or compare changes, and add:
  - `a_sphere_reaches_a_grid_reach_further`
  - `a_small_sphere_reaches_sixteen_radii`
  - `a_scroll_under_a_still_lamp_does_not_relight`
  - `a_new_picture_fades_in_over_sixteen_places`
  - `a_retaken_picture_keeps_its_readiness`
  - `a_scrolled_in_probe_is_not_ready`
- `render/tests/tests.md`: the `bounce_probes.c` entry names the new cases.

## Done when
`ctest --test-dir build/debug -R '^render/bounce_probes$'` passes, with the six new cases in
`render/tests/bounce_probes.c`.
