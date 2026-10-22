# 03 — The camera pass skips particles and water it cannot see
folder: 3d
after: 02
decisions: 0168, 0298, 0305, 0464

## Change
0464 points 2 and 5 for the two blended kinds `_run` holds, with the cull and counts card 02 made.

- `3d/src/draw_particles.h`, `3d/src/draw_particles.c` — `voe_3d_draw_particles_hold` gains
  `const struct voe_3d_cull *cull` (NULL tests nothing) and `voe_3d_view_counts *counts`: a live
  particle whose sphere (its place about the eye at the lag, radius half its size at its age × √2,
  the camera-facing quad's corner) is not seen is not held, `skipped` + 1; a held one `drawn` + 1.
  `voe_3d_draw_particles_count` is unchanged (it sizes the group's room).
- `3d/src/draw_water.h`, `3d/src/draw_water.c` — `voe_3d_draw_water_hold` gains the same two: a
  water's sphere is its turned, scaled quad's (centre its place about the eye, radius half the
  diagonal of width × length, plus its waves row's height), not held when not seen; counted the
  same way. The header says why the wave height is in the radius.
- `3d/src/draw_system.c` — `_run` hands both its cull and its counts.
- `3d/tests/draw_particles.c` — a case: an emitter behind the camera holds none of its live
  particles, counted as skipped; one ahead draws as before.
- `3d/tests/draw_water.c` — a case: a water behind the camera is skipped; a wide water whose middle
  is behind the camera but whose edge is ahead still draws its pixel.
- `3d/src/src.md`, `3d/tests/tests.md` — entries follow.

## Done when
Tests `3d/draw_particles`, `3d/draw_water` and `3d/view_culling` pass.
