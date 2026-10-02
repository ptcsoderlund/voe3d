# 07 — The frame's point lights carry slots for the nearest casting lamps
folder: 3d
after: 05, 06
decisions: 0168, 0325

## Change
0325 point 5, in the point-light fill; nothing opens a pass yet.
- `3d/include/3d/draw_system.h`: `#define VOE_3D_POINT_SHADOW_FADE 0.75f`, the share of D where a
  shadow starts fading. `voe_3d_draw_system_point_lights`'s comment: drop "are not shadowed"; say
  which lights get `shadow` and `shadow_strength` (ranked by the eye's distance to the light's
  sphere among the kept lights with `cast_shadows` and intensity above nought, slots 1..16 nearest
  first, strength 1 − smoothstep(FADE·D, D, distance) with D the 17th's, none at strength 0) and
  why this cannot pop.
- `3d/src/draw_point_lights.c`: after the walk, the choice over the kept array: the 17 nearest
  found by one pass keeping a short sorted list (no arena), slots and strengths written into the
  array. Header: the rule, its decision, that the shadows call reads the slots from `frame->points`.
- `3d/src/src.md`: `draw_point_lights.c`'s entry.
- `3d/tests/point_lights.c`, no card:
  - 20 casting lamps of range 5 along +X, 10 m apart from x = 10: the nearest 16 have distinct slots
    1..16 in order of distance, the rest 0; the nearest has strength 1, the 16th below 1, every
    strength within 0..1;
  - a lamp nearer than all with `cast_shadows` false, or intensity 0 casting, takes no slot;
  - with 16 casting lamps or fewer every one has strength 1;
  - the eye moved 0.1 m along +X changes no lamp's effective strength (0 for an unslotted lamp) by
    more than 0.05, at every step of a 20 m walk.
  Entry in `3d/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^3d/point_lights$"` passes.
