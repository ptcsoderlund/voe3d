# 24 — 3d holds the bounce at sun 1 and π, and its shadows without it
folder: 3d
after: 23
decisions: 0168, 0307, 0308, 0310, 0311
read: feature.md

## Change
Card 21's leftovers under 0310: `3d/bounce_scene`'s TINT ran at sun 9,
which clips, and `3d/shadows`' 100 KM OUT case reads bounce light under
the cube. Card 21 already fixed the casters' colour (`draw_shadows.c`).
Read `3d/tests/bounce_scene.c`, `3d/tests/shadows.c` and
`3d/tests/tests.md`.

- `3d/tests/bounce_scene.c`: THE TINT runs twice, sun 1 and sun π (fill
  as now), in place of sun 9; the claim becomes 0310's: ground 1 m out
  from the red box's lit face reads at least 12/255 more red than ground
  8 m from both boxes, in 8-bit. NO SPOTS stays, at both suns. The header
  says why not sun 9 (clips until a tone map, 0310) and that the gain is
  render's (0311).
- `3d/tests/shadows.c`: the 100 KM OUT case draws both of its pictures with
  the bounce off, the shadows call naming a target other than the one the
  pass draws into, as `bounce_scene.c`'s reference frame does; its claim
  stays "the cascades lose nothing at 100 km". The header says so.
- `3d/tests/tests.md`: both entries.
- If THE TINT fails at either sun while card 23's tank captures met 0310,
  block with the measured pixels; the owner is render's gain.

## Done when
The tests `3d/bounce_scene`, `3d/shadows`, `3d/bounce` and
`3d/bounce_grid` pass after the folder's build.

The human, in the editor on this machine (bug 02, How to reproduce, and
feature.md How to test 1–3): the tank project with the sun at 1 and at π
shows the ground at the purple box's sunlit foot plainly purple, fading
with distance, and no dark spots; turning the sun away fades it within a
second.
