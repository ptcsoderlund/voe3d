# 94 — The bounce times its parts by the frame's clock
folder: 3d
after: none
decisions: 0168, 0458, 0461, 0462, 0463

## Change
Bug 12, the bounce in 3d (0463 point 3): `shadows: bounce` took 538.7 of
543.1 ms on the CPU and nothing says on which of its six kinds of work.

- `3d/include/3d/draw_system.h`
  - `voe_3d_shadow_times` gains, after `bounce`, six `double` seconds,
    `bounce_fit`, `bounce_stale`, `bounce_begin`, `bounce_captures`,
    `bounce_suns`, `bounce_relight`, each within `bounce`, summed over the
    volumes, and a `uint32_t bounced`, the bounce calls made with a clock.
    A comment per field, as the five have.
  - `voe_3d_draw_system_shadows`'s comment, the timing point: the bounce
    parts and `bounced`, what each holds, that the rest of the bounce call
    is in none, and that the bounce's caster walks add nothing to `ground`.
- `3d/src/draw_bounce.h`
  - Declares the two clock helpers `3d/src/draw_shadows.c` keeps static
    now (`clock_read`, `clock_add`), renamed `voe_3d_draw_clock_read` and
    `voe_3d_draw_clock_add`, so both files time the same way.
  - `voe_3d_draw_bounce`'s comment: with `frame->clock` it times its six
    parts into `frame->shadow_times` and adds one to `bounced`; without
    one nothing is read or written.
- `3d/src/draw_shadows.c` — the two helpers made non-static under their new
  names, their callers following; nothing else changes.
- `3d/src/draw_bounce.c` — in `voe_3d_draw_bounce` and its helpers, with a
  clock, read it around:
  - `bounce_fit`: `voe_3d_bounce_box` and `voe_3d_bounce_grid_fit`;
  - `bounce_stale`: `voe_3d_bounce_stale` and `voe_3d_bounce_removed`;
  - `bounce_begin`: each `voe_render_bounce_begin`;
  - `bounce_captures`: each capture pass, its begin, caster draws and end;
  - `bounce_suns`: each `draw_sun_map`;
  - `bounce_relight`: each `voe_render_bounce_relight`;

  and add one to `bounced` per call. The file header gains one point on
  the timing (0463).
- `3d/tests/shadow_times.c` — new case `shadow_times_bounce`: the sun's
  light row with `bounces` 1 and `bounce_strength` 1 (read the header of
  `3d/tests/bounce_world.inc` and its `a_world` for how a sun bounces and
  what a bouncing frame needs), the fake clock of `shadow_times_casters`,
  one shadows call. Checks: `bounced` is 1; `bounce_fit`, `bounce_stale`,
  `bounce_begin` and `bounce_relight` each above 0; the six parts summed
  below `bounce`. `shadow_times_casters` also checks `bounced` is 0 and
  `shadow_times_untimed` that every new field is 0.
- `3d/3d.md` (`draw_system.h`), `3d/src/src.md` (`draw_bounce.h`,
  `draw_bounce.c`) and `3d/tests/tests.md` (`shadow_times.c`): each entry
  follows, as a phrase.

## Done when
`ctest --test-dir build/debug -R '^3d/shadow_times$'` passes (or reports
its skip without a device), and `grep -c "bounce_captures" 3d/src/draw_bounce.c`
prints at least 1.
