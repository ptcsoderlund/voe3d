# 03 — The shape system takes a new kind and re-points the mesh
folder: 3d
decisions: 0168, 0190, 0191, 0195, 0198

## Change
`3d/src/shape_system.c`, in the drain and the run only:

- The drain stops putting `kind` back. A submitted `kind` that is one of `VOE_3D_SHAPE_CUBE`,
  `VOE_3D_SHAPE_CAPSULE` or `VOE_3D_SHAPE_CYLINDER` lands in the row as the colour does. A submitted `kind` that
  is none of the three is put back to the entity's own and reported as a corrected intent, through the same
  edge-triggered report the correction already uses — the colour's clamp keeps its behaviour and its report
  unchanged.
- After the pass that gives a mesh and a material to a shape that has neither, one more pass over the shape
  table: for each row whose entity has a mesh (`voe_3d_mesh_get`) whose `geometry` is one of the three the
  `voe_3d_shapes` holds and is not the one its `kind` names, `voe_3d_mesh_set_geometry` to the kind's. A mesh on
  any other geometry — an imported model's — is left alone, and so is a row whose kind names no geometry. Reuse
  the geometry-per-kind pick the run already has rather than writing the three-way choice a second time, and
  ignore nothing the call answers: a false from `voe_3d_mesh_set_geometry` on an entity that was just read to
  have a mesh is the assert the run already makes for the caller's own sizing.

`3d/include/3d/shape_system.h`: the `THE INTENT CARRIES THE WHOLE ROW` paragraph says instead that the drain
takes a new `kind` among the three built-in ones and puts an unknown one back, reported as a corrected intent
(0195, 0198). Add to `voe_3d_shape_system_run`'s own paragraph and to its comment that A KIND THAT CHANGED
RE-POINTS THE ENTITY'S MESH: why it is a re-point and not a mesh dropped and rebuilt (both meshes are ranges in
the same pools and the material is the one white one every kind wears, so there is nothing to free and nothing to
queue); why the pass is over the table each run and remembers nothing, which is what keeps it idempotent and
means it also corrects a row written by a scene file; that a mesh on geometry none of the three shapes owns is an
imported model's and is never touched, which is the same promise as leaving an entity that already has a mesh
alone; and that the new kind is on the screen the same frame the run happens, no frame late.

`3d/tests/shape.c`, in the half that has a device: add that an intent naming another kind lands — a cube given
one mesh by a run, an intent of `VOE_3D_SHAPE_CYLINDER`, a second run, and the entity's mesh geometry is the
cylinder's while its material, its layer and its colour are what they were — and that an intent naming kind 99
leaves the row's kind and the mesh's geometry as they are. Compare geometries the way the test's existing
per-kind geometry check compares them.

`3d/tests/tests.md`: `shape.c`'s line says an intent's new kind lands and re-points the mesh, in place of "with
kind put back".

## Done when
`checks.sh` for `3d` exits 0 with the new checks in `3d/shape` on this machine's card, and `cmake --build
--preset debug` builds the whole tree.
