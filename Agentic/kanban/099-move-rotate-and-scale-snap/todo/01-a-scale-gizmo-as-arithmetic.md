# 01 — A scale gizmo as arithmetic
folder: 3d
after: none
decisions: 0168, 0482, 0205, 0274

## Change
0482 points 1 and 2: the scale gizmo's geometry, hit, grab and factor, beside the arrows and the
rings. Nothing draws it yet (card 03).

- `3d/include/3d/gizmo.h` — `voe_3d_gizmo_handle` gains `VOE_3D_GIZMO_CENTRE` after `ZX`; its
  comment says only the scale gizmo has one. Arrows and rings answer it as a handle they do not
  have.
- `3d/src/gizmo.c`, `3d/src/gizmo_rings.c` — read their headers only; where a function takes a
  handle and a check or table would let CENTRE through, it is answered as NONE is (grab, angle
  false; marked drawn as nothing marked). Touch the bodies only if needed.
- `3d/include/3d/gizmo_scales.h` (new), modelled on `gizmo_rings.h`; `turn` is the entity's world
  rotation (`voe_math_quat`), the axes the columns of `voe_math_float4x4_from_quat(turn)`:
  - constants: tip square half side 0.08, centre square half side 0.12 (fractions of a shaft),
    least factor 0.01, and `VOE_3D_GIZMO_SCALE_VERTICES` / `_INDICES` for three shaft quads,
    three tip squares and the centre square;
  - `voe_3d_gizmo_handle voe_3d_gizmo_scales_hit(voe_3d_gizmo gizmo, voe_math_quat turn,
    voe_3d_ray ray)` — X, Y, Z or CENTRE, nearest along the ray; NONE as the rings' says;
  - `[[nodiscard]] bool voe_3d_gizmo_scales_grab(voe_3d_gizmo gizmo, voe_math_quat turn,
    voe_3d_gizmo_handle handle, voe_3d_ray ray, voe_math_double3 *out)` — an axis's nearest
    point, the centre's crossing of the plane facing the eye; false as `voe_3d_gizmo_grab` is;
  - `float voe_3d_gizmo_scales_factor(voe_3d_gizmo gizmo, voe_math_quat turn,
    voe_3d_gizmo_handle handle, voe_math_double3 grab, voe_math_double3 now)` — 0482 point 2;
  - `[[nodiscard]] bool voe_3d_gizmo_scales_quads(voe_3d_gizmo gizmo, voe_math_quat turn,
    voe_3d_gizmo_handle marked, voe_base_arena *arena, voe_3d_gizmo_mesh *plain,
    voe_3d_gizmo_mesh *marked_out)` — as the rings' quads, built with `src/gizmo_quads.h`.
  Header points: why the entity's own axes (0482), why measured from the press, why the centre
  reads travel up the plane facing the eye, why the factor has a floor.
- `3d/src/gizmo_scales.c` (new) — the above, float about the ray's origin and the eye (0250).
- `3d/tests/gizmo_scales.c` (new) — rays onto each tip and onto the centre answer X, Y, Z,
  CENTRE, and beside them NONE; a quarter turn about Y puts X's tip on world −Z; an axis grabbed
  at one shaft and now at two gives 2; the centre grows dragged up, shrinks dragged down, stops
  at 0.01; quads fill exactly the constants; a shaft of nought hits nothing and builds nothing.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries for the new files; `gizmo.h`'s
  entry names the centre handle.

## Done when
`ctest --test-dir build/debug -R '^3d/gizmo'` passes, `3d/gizmo_scales` among them.
