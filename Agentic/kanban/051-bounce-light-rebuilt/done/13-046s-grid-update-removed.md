# 13 — 046's grid update, grids and schedule are removed
folder: render
after: 11
decisions: 0168, 0317, 0326

## Change
0326 point 9, first half; nothing calls these since card 11. The bounce map and pass go in 14.
- `render/include/render/device.h`: `struct voe_render_bounce_update`, `voe_render_bounce_update`,
  `VOE_RENDER_BOUNCE_PROBES`, `_BUDGET` and `_BLEND` go, with every comment naming them (the file
  header's bounce sentence; the `targets` paragraph's 786 kB grid, now the volume of card 05).
- `render/src/bounce_grid.c`, `render/src/bounce_schedule.h`, `render/src/bounce_schedule.c`,
  `render/shaders/bounce.slang`: deleted.
- `render/src/target.c`, `render/src/target_own.c`: the old grid's build, teardown and the
  struct member go; the window's too; headers lose their grid paragraphs.
- `render/src/device_parts.h`: `voe_render_bounce_mark`, `voe_render_bounce_grid`, the
  `bounce_schedule.h` include and the slot's VPL, list and set members go.
- `render/src/device_internal.h`: the update's pipelines, layout, pool and calls, the grid build
  and teardown, `window_grid`.
- `render/src/device.c`: their startup and shutdown.
- `render/src/loader.h`: an entry only the update used, if any (the header says which).
- `render/src/frame.c`, `render/src/pass.c`: any reference left.
- `render/tests/bounce_grid.c`, `render/tests/bounce_schedule.c`: deleted.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`,
  `render/render.md`: entries go or change.

## Done when
`! grep -rn "bounce_schedule\|voe_render_bounce_update\|VOE_RENDER_BOUNCE_BUDGET\|bounce\.spv" render --include=*.c --include=*.h --include=*.slang*`
exits 0, and `ctest --test-dir build/debug -R "^render/bounce_"` passes.
