# 61 — A removed light blocker relights the one-metre nest
folder: render
after: none
decisions: 0168, 0389

## Change
Card 59 moved the 3d bounce scene's eye to 5 m up, 8 m back, so the 1 m nest covers the box (0390); that
is in the tree (commit 95fb13e2) and every case passes but BLOCKED's last check: a light blocker over the
ground patch beside the box's lit face, settled, then destroyed and settled, leaves the patch reading as no
bounce (98 98 98 against 106 98 98 before the blocker), still 200 frames later. With the old eye, where the
level grid alone reads the patch, it recovers. So volume 0 relights after a blocker goes and the nest does
not, though 3d begins every volume with the same blockers (`3d/src/draw_bounce.c` header). By 0389 point 5
a different blocker count is a lights change: the nest must relight whole. Find and fix that in render.

- `render/tests/blocked_bounce.c`:
  - New case `a_removed_blocker_relights_the_nest`, in the sun-2 scene with its blocker round the patch:
    every frame begins volume 0 (spacing 2, as now) and volume 3, the 1 m nest, about the same place,
    lowest cell (−12, −6, −12), corner (−12, −6, −12), as `render/tests/bounce_read.c`'s THE NESTS does
    (read its header for how), both with the same blockers. Settled without the blocker, the patch pixel
    read; settled with it, within 2/255 of bounces 0; the blocker gone (count 0) and settled, within
    1/255 of the first reading. Settled is as the file settles now, with every begun volume relighting
    nothing.
  - Header: a paragraph for the case.
- Find why the nest keeps its blocked light, by the headers of `render/src/bounce_probes.h` (relight
  needed, per volume), `render/src/bounce_volume.c` (begin, place), `render/src/bounce_relight.c` (whole
  relight, the record's blockers) and `render/shaders/bounce_relight.slang`, then the body of the one at
  fault. Fix it there, in its owner only; correct that file's header if it said otherwise.
- `render/tests/tests.md`: the `blocked_bounce.c` entry names the new case.
- `render/src/src.md` or `render/shaders/shaders.md`: the fixed file's entry, if what it says changed.

If the new case passes with no fix, the fault is 3d's: block, giving both readings of the 3d test's
`unblocked patch` and of the new case, and leave the case in.

## Done when
`ctest --test-dir build/debug -R '^render/(blocked_bounce|bounce_)'` passes with
`a_removed_blocker_relights_the_nest`; then, after `cmake --build --preset debug --target
voe_test_3d_bounce_scene`, `ctest --test-dir build/debug -R '^3d/bounce_scene$'` passes.
