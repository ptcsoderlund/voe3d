# 07 — Lit surfaces read the bounce
folder: render
after: 06
decisions: 0168, 0273, 0275, 0307, 0308

## Change
0308 point 6. Read `render/shaders/lighting.slangh`,
`render/shaders/draw.slang` (the camera block's bounce record and the 3D
image binding from card 05), `render/shaders/shaders.md`,
`render/tests/shadow.c` (how a test builds a light view and a lit scene)
and `render/tests/bounce_grid.c`.

- `lighting.slangh`: a function giving irradiance E(n) at a position about
  the eye from the grid: texture coordinate = (lowest cell mod 32 +
  (p − corner) / spacing + 0.5) / 32, sampled trilinear with repeat from the
  three images; E = A0·L00 + A1·(L1 · n) per channel, A0 = π, A1 = 2π/3,
  clamped at 0; faded to nothing over the outer cell of the grid on each
  side; 0 when the record's index is ~0u.
- The lit fragment: lit = direct + base × max(E/π, fill weight × fill), per
  channel, where the fill weight is the existing 1 − smoothstep(0, 0.3,
  reach). Unlit and unshaded paths are unchanged; water reads the same
  function where it reads the fill.
- The header comment of `lighting.slangh` and the `shaders.md` entry say
  the fill is now a floor under the bounce (0307 amends 0275).
- `render/tests/bounce.c`, new, headless: grey ground (a large flat box, base
  0.5) and a red wall (base 1, 0, 0) standing on it, a sun from the wall's
  ground side at 45°, fill 0, four cascades, a bounce pass over the scene
  with the casters drawn, the update for the window with the grid around the
  wall, then a camera pass looking at the ground beside the wall. Cases:
  - a ground pixel 1 m from the wall has red over green greater than one
    10 m away, and greater than the same pixel in a frame with no update;
  - with no update, the ground is grey (red equals green within 2/255);
  - an unshaded pass draws the same ground identically with and without the
    update;
  - with the fill at 0.5 and no update, a shadowed ground pixel equals the
    fill as before (the floor holds).
- `render/tests/tests.md`: the entry for `bounce.c`.

## Done when
The test `render/bounce` passes, and `render/shadow`, `render/water`,
`render/unshaded` and `render/bounce_grid` still pass, after the folder's
build.
