# 07 — A world's fog volumes become a pass's records
folder: 3d
after: 01, 02
decisions: 0168, 0428, 0480, 0493

## Change
0493 points 1 to 3 on 3d's side: each Fog volume row's box placed by its transform, and the rows
as render's records about an eye. Nothing calls it until card 08. Read
`3d/include/3d/light_blocker.h` and `3d/src/light_blocker.c` (the box from a row and a
transform), the headers of `3d/src/draw_light_blockers.c` (rows about the eye, the off owner and
the cap) and `3d/src/draw_clouds.c` (a drift wrapped in double, no table and no drift row),
`3d/tests/clouds.c` and the header of `3d/tests/light_blockers.c` as test patterns,
`scene/include/scene/fog_volume_component.h` (card 01), and `voe_render_fog_volume` and
`voe_render_fog` in `render/include/render/device.h` (card 02).

- `3d/include/3d/fog_volume.h` (new):
  - `[[nodiscard]] bool voe_3d_fog_volume_shape(const voe_ecs_world *world, voe_ecs_entity
    entity, float lag, voe_physics_shape *box)`: the row's box as a light blocker's is, its centre
    the world place at the lag in double, its rotation the world rotation, its half |scale| ×
    size / 2; false with no row or no transform.
  - `voe_render_fog voe_3d_fog_volumes(const voe_ecs_world *world, voe_math_double3 eye, float
    lag)`: every row whose entity is on and has a transform, in table order, at most
    VOE_RENDER_FOG_VOLUMES; a box with a half of nought on an axis, or thickness 0, left out; each
    record's rows about `eye` as a light blocker's rows are; `density` 0.1 × thickness²;
    `patchiness` and `godrays` (the row's godrays strength) as authored; `patches` eye − drift and
    `change` eye − (1.5 × drift x, 0.25 × |drift|, 1.5 × drift z), each wrapped into 0 to
    VOE_RENDER_FOG_PATCHES and 0 to VOE_RENDER_FOG_CHANGE per axis in double, no drift row read as
    0; zeroed with no table or no row.
  Header points: one box for the pass and the editor's lines, as the light blocker's; why the
  density is squared (a slider's low end is thin fog); why the fields wrap in double; that the
  pass's fog and an editor view with its own eye both take it.
- `3d/src/fog_volume.c` (new): both calls, with its header.
- `3d/tests/fog_volumes.c` (new), no graphics card: no table and no row are zeroed; a row at
  (10, 2, −30), size (20, 4, 20), scale 1, eye at the origin gives one record whose rows hold the
  eye-relative centre at unit place 0 and the point 10 m from it along +X at unit x 1; scale 2 doubles the
  half; thickness 0.5 gives density 0.025; a drift of x 10 at eye x 5 gives patches x 123 and
  change x 22 (5 − 15 + 32); eye x 300 with no drift gives patches x 44; an off row and a
  thickness-0 row are left out; nine rows give eight records; the shape's half is |scale| ×
  size / 2.
- `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: an entry for each new
  file beside the light blocker's, each at most 300 characters.

## Done when
The test `3d/fog_volumes` passes after the folder's build.
