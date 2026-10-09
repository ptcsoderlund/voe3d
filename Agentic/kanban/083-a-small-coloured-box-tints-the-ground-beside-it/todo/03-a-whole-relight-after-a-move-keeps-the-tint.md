# 03 — A whole relight after a move keeps the nest's tint
folder: 3d
after: 02
decisions: 0168, 0389, 0390, 0393

## Change
082's open fault, in 3d's frame: the red box moved 1 m +x and settled reads its 1 m nest patch at
`106 98 98`; a whole relight then (bounce strength 1 → 2 → 1, each settled, or a light blocker added and
destroyed) leaves it `98 98 98`, the tint gone. Card 01 proved render relights each volume right under
its own sun map (or fixed it there; `git log -1 --grep='083 card 01'` says which).

- New `3d/tests/bounce_tint.c`, a test program including `bounce_world.inc`. Header: what the program
  proves, each case one paragraph, the decisions it follows. Case
  `a_whole_relight_after_a_move_keeps_the_tint`: the world as `bounce_scene.c` builds it, settled; the
  box moved 1 m +x, settled; the patch read; strength 1 → 2 → 1, each settled; the patch within 1/255 of
  the read before in every channel, and redder than the reference world's by at least TINT/255. Print the
  three patches on failure.
- `3d/tests/bounce_scene.c`, case BLOCKED: restore the check that the tint comes back once the blocker is
  destroyed and settled (0393): within BLOCKED/255 of the patch before the blocker. Its header paragraph
  and the comment over the case lose "until work order 083".
- Find the fault and fix it in 3d. Read headers first, then the body of the one at fault:
  `3d/src/bounce_grid.c` (`voe_3d_bounce_grid_sun`, the nest's sun view: centre, radius, snap),
  `3d/src/draw_bounce.c` (which casters are drawn into each volume's sun map, in what order, and what a
  probing frame whose pass is refused leaves marked), `3d/src/draw_shadows.c`, `3d/src/draw_bounce.h`.
  One suspect: a whole relight that runs while the nest has not finished recapturing after the move, so
  render relights it from probes it is about to replace, or a probing frame that counts as the relight
  done. Fix it in its owner; correct the header that said otherwise, and its `3d/src/src.md` entry if what
  the file does changed.
- If the fault is render's after all, stop: block the card naming the render function and a repro to add
  to `render/tests/blocked_bounce.c`.
- If both checks pass with no fix (card 01's fix closed it), the card is done; say so in the commit
  message.
- `3d/tests/tests.md`: an entry for `bounce_tint.c`; the `bounce_scene.c` entry's BLOCKED line says the
  tint coming back is checked.

## Done when
After `cmake --build --preset debug --target voe_test_3d_bounce_tint voe_test_3d_bounce_scene`,
`ctest --test-dir build/debug -R '^3d/bounce_(tint|scene)$'` passes.
