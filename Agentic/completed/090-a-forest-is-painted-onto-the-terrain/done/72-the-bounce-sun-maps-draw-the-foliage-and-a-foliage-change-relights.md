# 72 — The bounce sun maps draw the foliage, and a foliage change relights
folder: 3d
after: 71
decisions: 0168, 0442, 0450, 0451

## Change
Bug 04, 3d's half: the forest floor is in its trees' shade in the bounce.
The trees' shade is in the cascades (`draw_shadows.c`) but `draw_bounce.c`
draws only the casters into each sun's relight map, and an erase marks no
stale sphere, so nothing relights. Fixed once, in the bounce.

- `3d/src/models_store.h` — `struct voe_3d_models` gains `uint32_t
  changes`, 0451 point 3's count; the header's first paragraphs say what
  raises it and who reads it (the bounce).
- `3d/src/models.c` — raised wherever an entry is put loaded, replaced or
  cleared.
- `3d/src/models_landscape.c` — raised at a landscape's load and wherever
  its copies are placed again over changed heights (brush and put).
- `3d/src/models_foliage.c` — raised when a scatter or erase changes a
  copy, on every put of copies and every kind set. Header line follows.
- `3d/src/draw_bounce.c` — every `voe_render_bounce_begin` gets `.shade`
  = the frame's store's `changes`, nought with no `frame->models`. In each
  sun's relight map, after `voe_3d_draw_casters`,
  `voe_3d_draw_foliage_cast(device, frame)` when that sun's blocker mask
  (`within`) is nought, as `draw_light_cascades` does for a cascade; false
  from it stops as a refused caster draw does.
- `3d/src/draw_bounce.h` — `voe_3d_draw_bounce`'s comment: each sun map
  draws the foliage when no blocker holds its sun, and the begin's shade.
- `3d/src/draw_foliage.h` — the "FOLIAGE IS NOT IN THE BOUNCE" paragraph
  becomes: in the bounce sun maps (0450), not in captures or point shadows,
  and why.
- `3d/include/3d/draw_system.h` — `voe_3d_draw_system_shadows`' comment:
  a relight map draws the foliage groups too, one object each, and a
  store change relights; reworded within the cap.
- `3d/tests/bounce_foliage.c` — new, headless, skips without a card. The
  world of `3d/tests/draw_foliage.c` (its store, `.glb`, flat landscape
  with three copies and camera above; its device with room for the passes
  `bounce_world.inc` sizes) with a light row from above casting
  and bouncing 1, and a `voe_3d_bounce_casters` memory; each frame
  `voe_3d_draw_system_foliage`, then `voe_3d_draw_system_shadows`, its
  draw count read by `voe_render_frame_draw_count`. Settle it by passes
  as `3d/tests/bounce_world.inc`'s `settles` does (its method, not its box
  world), and read a settled frame's count. Then: a kind set naming kind
  0's own path (`voe_3d_models_landscape_kind_set`) makes the next frame
  draw d1 more than the settled one; settled again, erasing one copy
  (`voe_3d_models_landscape_erase`) makes the next frame draw more; erase
  the rest, settle, and the same kind set gives d2. d1 − d2 is a positive
  whole multiple of the first frame's groups not `rest`.
- `3d/src/src.md`, `3d/tests/tests.md` — entries for `models_store.h`,
  `models_foliage.c`, `draw_bounce.*`, `draw_foliage.h` and the new test.

## Done when
Tests `3d/bounce_foliage`, `3d/draw_foliage`, `3d/bounce`,
`3d/bounce_tint`, `3d/bounce_pacing` and `3d/shadow_lights` pass.
