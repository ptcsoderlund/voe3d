# 03 — A brush scatters and erases copies
folder: 3d
after: 01, 02
decisions: 0168, 0442

## Change
0442 points 2 and 3: the foliage arithmetic on a landscape's copies, CPU only, beside the
sculpting stamps.

- `3d/include/3d/landscape.h` — `voe_3d_brush_kind` gains `VOE_3D_BRUSH_SCATTER` and
  `VOE_3D_BRUSH_ERASE` after `_PAINT`; `voe_3d_landscape_brush` asserts on either (they are
  `3d/foliage.h`'s). Its brush weight, if static in `3d/src/landscape.c`, moves to an internal
  header `3d/src/brush_weight.h` both files include, taking radius and softness.
- `3d/include/3d/foliage.h` — new public header:
  - `voe_3d_foliage_brush`: `radius`, `softness`, `density` (per 100 m²), `size_vary` (0..1),
    `turn_vary` (radians, 0..π), `lean` (0..1), `picked` (bit k: kind k).
  - `VOE_3D_FOLIAGE_TRIES` (256): the most points one scatter stamp tries.
  - `voe_scene_transform voe_3d_foliage_place(const voe_assets_landscape *, voe_assets_landscape_copy)`:
    the copy in the grid's own space as 0442 point 2 says; up from the ground's normal by
    central differences one cell apart, tilted by `voe_math_quat_from_axis_angle` about
    normalize(+Y × n) by acos(n.y) × lean / 255, then the turn about +Y, scale uniform.
  - `uint32_t voe_3d_foliage_scatter(voe_assets_landscape *, voe_scene_transform *placed,
    const voe_3d_foliage_brush *, float x, float z, float seconds, uint64_t *seed,
    voe_assets_landscape_copy *added)`: one stamp as 0442 point 3, xorshift64 on `*seed`
    (nonzero, advanced); points off the square are not kept; the copies near the stamp
    (within its box widened by the spacing) gathered once, then each try checked against
    them and the kept ones, never a scan of every copy per try. Appended to `copies` up to
    VOE_ASSETS_LANDSCAPE_COPIES, each placed into `placed[i]` when `placed` is not NULL, and
    written to `added` (room VOE_3D_FOLIAGE_TRIES); returns how many. None picked adds none.
  - `uint32_t voe_3d_foliage_erase(landscape, placed, brush, x, z, voe_assets_landscape_copy
    *removed, uint32_t room)`: every copy of a picked kind within the radius, at most `room`,
    removed by moving the last into its place (`placed` alike) and written to `removed`.
  - `void voe_3d_foliage_put(landscape, placed, const voe_assets_landscape_copy *, uint32_t
    count, bool add)`: undo's: appended (to the cap) and placed, or each copy equal in every
    field removed as erase removes; one not found is passed over.
  - `void voe_3d_foliage_replace(const voe_assets_landscape *, voe_scene_transform *placed,
    voe_3d_landscape_rect heights)`: every copy within the rect's points widened by one cell
    placed again.
  - The caller's `copies` has room for VOE_ASSETS_LANDSCAPE_COPIES (asserted not NULL).
  - Header points: what 0442 points 2–3 say, why the spacing makes a held brush stop near
    the density, why erase keeps no order.
- `3d/src/foliage.c` — the above.
- `3d/tests/foliage.c` — new, on a flat 16-cell 64 m grid with two kinds and room for the cap:
  a stamp of radius 10 at the centre, density 4, kind 1 picked, held 100 stamps of 0.1 s, adds
  copies all of kind 1 within 10 m, between 6 and 16 of them, no two nearer than the spacing; a
  seed run twice gives the same copies; erase of kind 0 removes none, of kind 1 all and returns
  them; `_put` of those back restores the count, `_put` remove takes them again; on a 45° plane
  a copy of lean 255 is turned 45° and of lean 0 upright, its y on the plane; `_replace` after
  raising a height under a copy moves its placed y.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries for the new files; landscape.h's
  entry names the two kinds.

## Done when
Tests `3d/foliage` and `3d/landscape` pass.
