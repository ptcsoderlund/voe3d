# 02 — The gizmo's size, what a ray meets, and where it grabs
folder: 3d
decisions: 0168, 0205

## Change
New `3d/include/3d/gizmo.h`, `3d/src/gizmo.c` and `3d/tests/gizmo.c`: the move gizmo as arithmetic, with
nothing drawn yet. Read `3d/include/3d/outline.h` for how a size in pixels becomes a size in metres and
`3d/include/3d/pick.h` for the ray.

```c
typedef enum {
	VOE_3D_GIZMO_NONE = 0, VOE_3D_GIZMO_X, VOE_3D_GIZMO_Y, VOE_3D_GIZMO_Z,
	VOE_3D_GIZMO_XY, VOE_3D_GIZMO_YZ, VOE_3D_GIZMO_ZX
} voe_3d_gizmo_handle;

typedef struct {
	voe_math_float3 origin;  // where the gizmo stands, in metres
	voe_math_float3 eye;     // the camera's position, in metres
	float shaft;             // one arrow's shaft, in metres
} voe_3d_gizmo;

voe_3d_gizmo voe_3d_gizmo_at(voe_math_float3 origin, voe_render_view view,
			     voe_platform_size size, float pixels);
voe_3d_gizmo_handle voe_3d_gizmo_hit(voe_3d_gizmo gizmo, voe_3d_ray ray);
[[nodiscard]] bool voe_3d_gizmo_grab(voe_3d_gizmo gizmo, voe_3d_gizmo_handle handle,
				     voe_3d_ray ray, voe_math_float3 *out);
```

Every other measurement is a `#define` fraction of `shaft`, so the whole gizmo has one size: the head's length
and half-width, the plane square's near corner and its side, the label's height, how far off an axis still
counts as a hit, and the step a marked handle grows by (card 03 uses that one). Write them as numbers with
the reason for each in one clause.

`_at` makes `shaft` the metres that cover `pixels` pixels at the origin's own depth: the origin through
`view.view`, its depth along the camera's forward direction, floored at a small positive number so a gizmo at
the eye is not a division by nought; then outline.h's formula, `pixels * 2 * depth / (projection.m[1][1] *
size.height)`. That is the whole of "the same size on screen however far away".

`_hit` answers what is under the ray: each axis as the segment from the origin to the far end of its head,
hit when the ray passes within the off-axis distance of it and the closest approach is in front of the eye;
each plane square as the ray's meeting with that plane, hit when it lands inside the square. Among hits the
one nearest along the ray wins, so a square that overlaps an arrow takes the press where it is in front.
`VOE_3D_GIZMO_NONE` for no hit, for a gizmo behind the eye and for a shaft of nought.

`_grab` is where a drag is measured from: for an axis, the point on that axis line closest to the ray; for a
plane, where the ray meets that plane through the origin. False, `*out` untouched, for `NONE` and for a ray
too nearly parallel to the axis's plane or to the square's — a fold the caller answers by keeping the last
position.

The header makes these points: why one call answers both what is drawn and what is grabbed; why the size is
worked out once from the origin's depth rather than per vertex as an outline's is; that the axes are the
world's and a later card is what makes them an entity's own; why the plane squares are tested first by
distance rather than by order; what "in front of the eye" excludes; and that a refused grab is an ordinary
frame, not a failure.

`3d/3d.md` gains an `include/3d/gizmo.h` entry; `3d/src/src.md` and `3d/tests/tests.md` gain theirs.

## Done when
`checks.sh --folder 3d` exits 0 and `ctest --test-dir build/debug -R "^3d/gizmo$"` passes, with claims, none
needing a graphics card: a ray down each of the three arrows hits that arrow; a ray through the middle of
each square hits that plane; a ray into empty space hits nothing; the same camera twice as far away gives
twice the `shaft`; a grab on X keeps the origin's Y and Z; a grab on ZX keeps the origin's Y; a ray parallel
to a plane is refused.
