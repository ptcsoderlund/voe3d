# 40 — A bounce begin carries its grid's spacing, and every record uses the volume's
folder: render
after: 38, 39
decisions: 0168, 0326, 0332

## Change
0332 point 3, for bug 03 of 051: 3d will fit the grid to the level at 2 m × 2^k, so the spacing
is the begin's, not a constant. The 3d caller sets it in card 42; until then 3d's bounce tests
assert on a spacing of nought, which is expected.
- `render/include/render/device.h`: `struct voe_render_bounce_frame` gains `float spacing`, last:
  metres between this volume's probes, `cell` counted in them. The struct's comment and
  `voe_render_bounce_begin`'s say so; begin asserts a spacing not finite or not above nought.
  `VOE_RENDER_BOUNCE_SPACING`'s comment: the finest spacing, a grid's at 2^0 (0332); a volume's
  own is its begin's.
- `render/src/device_parts.h`: the volume keeps, per frame slot, the spacing beside the lowest
  cell and corner it already keeps.
- `render/src/bounce_volume.c`: `voe_render_bounce_begin` checks and keeps the spacing and passes
  it to the place call (card 39 left `VOE_RENDER_BOUNCE_SPACING` there).
- `render/src/bounce_capture.c`: a probe's centre (corner + (local + 0.5) × spacing) and the
  record's `bounce.spacing` from the begun volume's spacing.
- `render/src/bounce_relight.c`, `render/src/bounce_shadow.c`, `render/src/point_shadow.c`,
  `render/src/pass.c` (two places): every `bounce.spacing` or relight `spacing` written from
  `VOE_RENDER_BOUNCE_SPACING` is written from the spacing of the volume that record names. After
  this no `.c` in `render/src` names `VOE_RENDER_BOUNCE_SPACING`.
  Each file's header, where it names 2 m or the constant, to match.
- `render/tests/bounce_volume.c`, `bounce_probes_scene.c`, `bounce_settle.c`, `bounce_capture.c`,
  `bounce_read.c`, `bounce_shadow.c`: each frame record they build sets `.spacing =
  VOE_RENDER_BOUNCE_SPACING`. `bounce_volume.c` gains a case beside card 33's: the target's volume
  begun at spacing 4 and the window's at 2 in one frame, each relit; through
  `render/src/device_internal.h`, each region of that slot's relight record holds its own spacing.
  Header paragraph for the case.
- `render/src/src.md`, `render/tests/tests.md`: entries for the files whose role changed.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes and
`! grep -n "VOE_RENDER_BOUNCE_SPACING" render/src/*.c` exits 0.
