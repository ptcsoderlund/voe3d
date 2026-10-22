# 01 — A sphere is tested against a view's planes
folder: 3d
after: none
decisions: 0168, 0258, 0395, 0464

## Change
0464 point 1: the one place in 3d that says whether a sphere about the eye is inside a view.
Internal to the folder; nothing calls it yet.

- `3d/src/cull.h` (new) — `struct voe_3d_cull`: up to six planes (`voe_math_float4`, xyz the
  inward normal, w the offset) and their count.
  - `struct voe_3d_cull voe_3d_cull_of(voe_render_view view, bool sun_side_open)`: the planes of
    `projection × view` (eye-relative, 0250), left, right, bottom, top, near and far by this
    engine's clip space (reversed depth, near at 1, no Y flip: `3d/include/3d/projection.h`'s
    header); with `sun_side_open` the near plane is left out, which is how a cascade
    (`voe_3d_shadow_cascades_fit`'s `light[c]`) keeps casters beyond its sun side (0464 point 3).
    Each plane normalised so a distance is in metres.
  - `bool voe_3d_cull_sees(const struct voe_3d_cull *, voe_math_float3 centre, float radius)`:
    false only when the sphere lies wholly outside some plane.
  - `float voe_3d_cull_radius(voe_math_float4x4 world, float radius)`: `radius` times the longest
    of the upper 3×3's columns, a sphere's radius under a world matrix.
  - Header points: why a sphere (conservative, so nothing visible is skipped and nothing pops at an
    edge), why the planes are eye-relative, why a cascade's sun side is open, why it is 3d's and
    not render's (render cannot see a node's heights or a water's waves, 0464).
- `3d/src/cull.c` (new) — the plane extraction from the rows of the matrix, the normalise, the
  test and the radius.
- `3d/tests/cull.c` (new) — cases, each a named check:
  - a camera from `voe_3d_view` at the origin looking down −Z: a 1 m sphere 10 m ahead is seen;
    10 m behind is not; beyond the far plane is not; one whose centre is just outside the left
    edge but whose radius reaches in is seen;
  - a view made with `voe_3d_projection_orthographic` (a cascade's lens): with `sun_side_open` a
    sphere far beyond the near plane on the axis is seen, without it is not; one past a side is
    not either way;
  - `voe_3d_cull_radius` of a matrix scaled (1, 3, 2) is 3 × the radius.
- `3d/src/src.md`, `3d/tests/tests.md` — an entry for each new file.

## Done when
Test `3d/cull` passes.
