# 91 — The shadows call times its parts by a clock it is handed
folder: 3d
after: 87
decisions: 0168, 0458, 0461

## Change
Bug 11, the measurement in 3d (0461 point 2): a second goes in
`voe_3d_draw_system_shadows` with bounces at 0 and nothing says on what.

- `3d/include/3d/draw_system.h`
  - New `voe_3d_shadow_times`: five `double` seconds, `casters`, `ground`,
    `foliage`, `lamps`, `bounce`. Zeroed is none.
  - `voe_3d_frame` gains, last, `double (*clock)(void)` (NULL: nothing is
    timed; `voe_3d_draw_system_frame` leaves it NULL, a caller sets it, as
    with `models`) and `voe_3d_shadow_times shadow_times`, which only the
    shadows call writes.
  - `voe_3d_draw_system_shadows`'s comment: with a clock, it adds each
    part's seconds to `frame->shadow_times`; what each part holds; why a
    clock handed in (3d names no platform, 0461).
- `3d/src/draw_shadows.c` — with `frame->clock` set, read it around each
  part and add the difference: `casters` the shapes' and models' draws into
  every cascade, `ground` the landscape nodes' draws into them, `foliage`
  the `voe_3d_draw_foliage_cast` calls, `lamps` the point-shadow pass,
  `bounce` the probe bounce call. Cascade fitting and pass opens fall
  outside every part. Without a clock, no read and no write. The header
  says the timing in one point.
- `3d/src/draw_bounce.h` (read its header for the cascades' caster walk):
  if the ground is drawn inside that walk rather than by its own call, time
  `ground` where the walk draws a landscape row, through a pointer to the
  frame's times the walk is handed, and say so in the walk's comment; the
  file that defines the walk is the only other one changed.
- `3d/tests/shadow_times.c` — new. Copy the device and world setup of
  `3d/tests/shadows.c` (a cube under a casting straight-down sun, skipped
  without a device). Checks:
  - `shadow_times_untimed`: a frame with no clock leaves all five at 0.
  - `shadow_times_casters`: with a fake clock that returns one more second
    on each read, `casters` is above 0 and `bounce` is 0 (no light
    bounces).
- `3d/3d.md` (the `draw_system.h` entry), `3d/src/src.md`
  (`draw_shadows.c`) and `3d/tests/tests.md` (the new test): an entry
  each, as a phrase.

## Done when
`ctest --test-dir build/debug -R '^3d/shadow_times$'` passes (or reports
its skip without a device), and `grep -c "frame->clock" 3d/src/draw_shadows.c`
prints at least 1.
