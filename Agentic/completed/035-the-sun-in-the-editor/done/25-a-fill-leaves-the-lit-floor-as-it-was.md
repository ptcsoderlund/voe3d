# 25 — A fill leaves the lit floor as it was
folder: 3d
decisions: 0168, 0275, 0276

## Change
Needs card 23. Proves bug 02 fixed through `3d`'s whole draw, shadows included.

- `3d/tests/shadows.c`, the case where a fill of 0.2 white lifts the floor's shadowed patch
  under the cube: also read the lit floor beside the patch with and without the fill; it
  reads the same within the file's tolerance. The patch still reads lighter than without the
  fill and darker than the lit floor. Update the case's comment and its line in
  `3d/tests/tests.md`.
- `3d/include/3d/draw_system.h`: nothing changes (the fill still arrives as fill_colour times
  fill_intensity); leave it.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0.

The human's, not the coder's: open `examples/coin_game` in the editor, select the sun, set
`fill_colour` to a strong blue and `fill_intensity` to 1. A surface in full sun does not change;
shadowed sides and sides facing away turn lighter and blue, and the shadows stay (feature.md
step 5). Press Play: the game is lit the same way (step 8).
