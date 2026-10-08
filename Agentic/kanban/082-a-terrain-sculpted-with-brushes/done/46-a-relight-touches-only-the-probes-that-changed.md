# 46 — A relight touches only the probes that changed
folder: render
after: 45
decisions: 0168, 0386, 0387, 0388, 0389

## Change
0389 point 5: when the bouncing lights are unchanged, relight and sum run only over the listed changed
probes, not over all 6912.

- `render/src/bounce_relight.h`: the push block gains a `whole` flag. Keep the size assert true.
- `render/src/bounce_relight.c`:
  - With `voe_render_bounce_probes_lights_changed` true, the level and sum dispatches are as now, whole
    grid, `whole` set.
  - Otherwise, each dispatch is one workgroup per changed listed probe, `whole` clear.
  - New `device->relight_groups` adds up the workgroups of every relight dispatch, beside
    `relight_dispatches`.
- `render/src/device_internal.h`: `uint32_t relight_groups`, commented like `relight_dispatches`.
- `render/shaders/bounce_relight.slang`: the relight and sum entries take their probe from the group id
  when `whole`, and from the list's word at the group id (bits 0–15) when not.
- `render/src/bounce_relight.c` header, CONSTRAINTS: the fixed cost of 0326 holds only when the lights
  change. Levels 2 and 3 of an unchanged probe beside a new picture catch up at the next lights change
  (0389 point 5).
- `render/tests/bounce_probes_scene.c`, new cases:
  - `a_recapture_relights_only_what_it_changed`: after settling, mark one stale sphere. The next relight's
    `relight_groups` is under the whole grid's count.
  - `a_lights_change_relights_the_whole_grid`: a moved sun gives at least 6912 groups a level.
- `render/tests/tests.md`: the entry names them.

Per frame (0388), `bounce relight`: while things move, it costs the changed probes' groups, not 6912 per
level.

## Done when
`ctest --test-dir build/debug -R '^render/(bounce_|blocked_bounce|blocker_kinds_bounce)'` passes with the
two new cases.
