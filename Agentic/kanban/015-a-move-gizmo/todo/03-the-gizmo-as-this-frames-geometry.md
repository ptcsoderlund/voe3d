# 03 — The gizmo as this frame's geometry
folder: 3d
decisions: 0168, 0205, 0206

## Change
`3d/include/3d/gizmo.h`, `3d/src/gizmo.c` and `3d/tests/gizmo.c` gain the triangles. Nothing is uploaded and
nothing is drawn here: this builds two arrays in the caller's arena, exactly as `3d/outline.h` does — read it
for the winding rule, the normals and whose arena it all is.

```c
#define VOE_3D_GIZMO_VERTICES <n>   // the whole gizmo, both meshes together
#define VOE_3D_GIZMO_INDICES  <n>

typedef struct {
	const voe_render_vertex *vertices;
	uint32_t vertex_count;
	const uint32_t *indices;
	uint32_t index_count;
} voe_3d_gizmo_mesh;

[[nodiscard]] bool voe_3d_gizmo_quads(voe_3d_gizmo gizmo, voe_3d_gizmo_handle marked,
				      voe_base_arena *arena, voe_3d_gizmo_mesh *plain,
				      voe_3d_gizmo_mesh *marked_out);
```

What it builds, all of it in world metres, out of card 02's one `shaft` and its fractions:

- per axis, a shaft: a quad from the origin along that axis, widened along the unit perpendicular to the axis
  and to the direction from the shaft to the eye, so it faces the camera; where those two are nearly parallel,
  along any unit perpendicular to the axis, so an arrow pointed at the eye is a sliver and never nothing.
- per axis, a head: one triangle in that same plane, from the shaft's end to the point, as wide as the head
  fraction says — what makes it an arrow rather than a line.
- per plane, a square: two triangles lying in the plane itself, between the near corner and the far one on
  both of its axes, wound from whichever side of the plane the eye is on.
- per axis, a label at the tip, camera-facing quads a stroke each: two crossed for X, three for Y, three for Z
  (0206). Labels are always in `plain`, whatever is marked.
- every vertex's normal is the unit direction from that vertex to the eye, for the reason outline.h gives.

`marked` names the one handle drawn into `marked_out` instead of `plain`, at the marked step's widths — its
shaft wider, its head wider, its square grown about its own centre — and its lengths unchanged, so a marked
handle covers the same reach it is hit over. `VOE_3D_GIZMO_NONE` leaves `marked_out` an empty mesh, which is
a mesh the caller draws nothing from. False, with both untouched, for a shaft of nought.

The two constants are what the two meshes cost together: a program sizes `voe_render_capacities`' transient
vertices and indices from them and counts two transient ranges and two draws per pass.

The header gains: why the handles are camera-facing quads rather than solid cones — one size, one set of
triangles, and every arrow readable from any angle; why the plane squares are the one part that is not; why
the marked handle is a second mesh rather than a second colour on the same one (a drawn object's colour is
one record per draw, ADR-0191); and why a label is strokes here rather than a `text` block (0206).

`3d/src/src.md` and `3d/tests/tests.md` entries gain the build.

## Done when
`checks.sh --folder 3d` exits 0 and `ctest --test-dir build/debug -R "^3d/gizmo$"` passes, with claims added,
none needing a graphics card: the counts never exceed the two constants, for every handle marked and for
none; every triangle of the shafts, heads and labels faces the eye — its winding normal pointing the eye's
way — and so does every square, from either side of it; marking a handle takes that handle's triangles out of
`plain` and puts that many into `marked_out`; the total triangle count is the same however the eye is placed.
