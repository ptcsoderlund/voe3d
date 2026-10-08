# 56 — The 3d suite passes again with the nests
folder: 3d
after: 52
decisions: 0168, 0386, 0387, 0388, 0389

## Change
Card 54 is in the tree (commit 06661f1e): `voe_3d_draw_bounce` now begins the level grid and then each
nest finer than it, the four capture passes shared coarse to fine, and a relighting frame opens one sun
map per casting sun per relighting volume (0389 points 1 to 3). `3d/bounce` passes. Two older tests
still fail and keep `checks.sh --folder 3d` from 0: bring both to the engine as it now is. Tests only;
do not change `3d/src` or `3d/include`. If a test can only pass by an engine change, block and say which.

- `3d/tests/shadow_lights.c`:
  - Its `bounce_passes` budgets one sun map per light; it now aborts at a pass refused. Size the device
    as `3d/tests/bounce.c` does, from `VOE_RENDER_BOUNCE_VOLUMES`. Work out how many volumes this world
    begins (1 + the nests whose spacing is below its level grid's, `3d/include/3d/bounce_grid.h`) and
    make the "each casting sun that bounces draws its own map" case count that many sun maps per
    casting sun: one short is false, exact is true.
  - The "a moon bounces" case reads the floor on the third frame and fails `bounced > still`. Since
    0389 point 4 a probe fades in over `VOE_RENDER_BOUNCE_FADE` (16) places, so the third frame reads
    it barely lit. Run frames until the volume is built and then that many more before reading, and
    find whatever else keeps `bounced > still` false if that is not enough.
  - Header: the passes the bounce cases count and when the moon case reads.
- `3d/tests/bounce_scene.c`, its existing cases only (card 57 adds the new ones):
  - Size the device from `VOE_RENDER_BOUNCE_VOLUMES`. A full frame's budget is the cascades,
    `VOE_RENDER_BOUNCE_CAPTURE_PASSES` capture passes, a sun map per begun volume (this world's 40 m
    ground fits a 2 m level grid, so the 1 m nest also begins) and the window's.
  - The world registers `voe_3d_shape_changes_register` (`3d/include/3d/shape_system.h`). Every frame
    first calls `voe_scene_transform_remember`, as the editor does (`editor/src/world_step.h`).
  - `settles` also runs `VOE_RENDER_BOUNCE_FADE` more full frames once a probing frame is true. A
    frame that only fades opens no pass, so without this a read could land mid-fade.
  - MOVE (bug 03): an eye moved 1.5 m opens no capture pass, being inside the 1 m nest's two cells.
    The 6 m move and back settles, and then the pixels are within 1/255 of before.
  - FAR: settle at the far eye before checking that probing frames are true.
  - TURN, BLOCKED, SETTLING, EVEN, the lit side and the shadow stand as they are; fix only the waits.
  - Header: the WORLD, SETTLED, MOVE and FAR paragraphs match.
- `3d/tests/tests.md`: the `shadow_lights.c` and `bounce_scene.c` entries match.

## Done when
`ctest --test-dir build/debug -R '^3d/(shadow_lights|bounce_scene|bounce)$'` passes.
