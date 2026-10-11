# shaders

The engine's shaders and the only files `slangc` runs over: Slang compiled to row-major SPIR-V at
build time and `#embed`ded, so nothing is read at run time. A `.slangh` is a part, included by a
shader and never compiled alone.

- `bindings.slangh` — the part `draw.slang` includes first: the frame block, the records, set 0's
  bindings and the push constant, each matching its C struct.
- `draw.slang` — the mesh pipelines' entry points: placing, maps at the record's repeat, alpha, the
  lit and unlit exits, the point-shadow vertex stage and the capture pass's two.
- `point_shadow.slangh` — a point light's cube faces: the face of a vector and the clip position on
  it; draw and lookup share it.
- `blockers.slangh` — which light blockers hold a point and whether a segment passes them, twin of
  `src/light_blockers.c`.
- `bounce_read.slangh` — the bounce's one read: E(n) from a probe volume's eight nearest probes,
  weighted and normalised.
- `lighting.slangh` — the directional lights, their shadows and fills, the bounce and the binned
  point lights, each gated by the light blockers.
- `terrain.slangh` — a landscape node's vertex: the grid point morphed by distance, its height and
  normal from the height map.
- `water.slangh` — the water path, lit by directional light 0 alone: wave normals, fresnel and
  coverage from its depth.
- `elements.slang` — the element pipeline: a rectangle per instance from its vertex index, clipped,
  and a glyph's coverage at its edge.
- `bounce_relight.slang` — the relight's compute: `settle` a probe's validity and moments, `relight`
  its picture into a level's six-axis irradiance, and `sum`.
- `matrix_probe.slang` — writes three elements of a matrix out as colour, so a test can tell which
  layout slangc used.
