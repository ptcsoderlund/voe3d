# 17 — The bounce holds through scrolls and two views
folder: render
after: 16
decisions: 0168, 0307, 0308

## Change
Bug 01, the half of the editor's conditions card 16 does not reach: the grid
scrolls frame after frame with blend 0.5, and each editor view runs its own
bounce pass and update into the one per-slot bounce map in the same frame.
Read `render/tests/bounce_scene.c` (card 16), `render/src/bounce_grid.c`
(when the reduce runs, the VPL buffer and the sets per slot and grid),
`render/src/bounce_map.c`, `render/src/pass.c`, `render/tests/tests.md`.

- `render/tests/bounce_scene.c`, cases added (header comment gains each):
  - SCROLLING: twelve frames, both frame slots in turn, the lowest cell
    moving +1 in x each frame and the corner with it, the eye fixed; after
    them card 16's off-origin and never-darker claims still hold.
  - TWO VIEWS, ONE FRAME: a device with `targets` 2; in one frame a bounce
    pass from light view A and the update of target A, then a bounce pass
    from a light view B 40 m away (a different eye and grid) and the update
    of target B, then a camera pass on each. Each target's ground pixel 1 m
    from the wall equals, within 2/255, the same target drawn in a frame
    where only its own bounce pass and update ran.
- Make both pass. The owner is `bounce_grid.c`: the reduce runs for each
  bounce pass that precedes an update (not once a frame), its VPLs are about
  that pass's eye, and the second pass's map writes wait for the first
  update's reads; an update's set and probe list band are its grid's own.
  Fix what the cases show there; its header comment says it.
- `render/tests/tests.md`: the entry for `bounce_scene.c` names the cases.

If a case fails with the fault outside `bounce_grid.c`, `bounce_map.c` and
`pass.c`, block with the measured pixels.

## Done when
The test `render/bounce_scene` passes, and `render/bounce`,
`render/bounce_grid` and `render/bounce_map` still pass, after the folder's
build.
