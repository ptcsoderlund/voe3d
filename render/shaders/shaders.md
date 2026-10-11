# shaders

The engine's shaders and the only files `slangc` runs over: Slang compiled to row-major SPIR-V at
build time and `#embed`ded, so nothing is read at run time. A `.slangh` is a part, included by a
shader and never compiled alone.

- `bindings.slangh` — the part `draw.slang` includes first: the frame block with its sky and its harmonics, the
  records, set 0's bindings and the push constant with its `plain` word, each matching its C struct;
  the object record carries a landscape's paint and layers.
- `draw.slang` — the mesh pipelines' seven entry points: placing, maps at the record's repeat, a
  painted landscape's layers, alpha, the lit and unlit exits, hazed, exposed and tone mapped, the
  point-shadow vertex stage, the capture pass's two and the sky's full-screen two.
- `tone.slangh` — haze, exposure and tone at every mesh exit: mixed towards the sky at the horizon
  by distance, × 2^exposure, then Clip or Soft by PBR Neutral, from the pass's sky; a `plain` draw
  as authored.
- `sky.slangh` — the sky's radiance along a view, single scattering through a flat atmosphere, twin
  of `src/sky.h`.
- `point_shadow.slangh` — a point light's cube faces: the face of a vector and the clip position on
  it; draw and lookup share it.
- `blockers.slangh` — which light blockers hold a point and whether a segment passes them, twin of
  `src/light_blockers.c`.
- `bounce_read.slangh` — the bounce's one read: E(n) from a probe volume's eight nearest probes,
  weighted and normalised.
- `lighting.slangh` — the directional lights, their shadows and fills, the bounce, the binned
  point lights and the sky's light and reflection, each gated by the light blockers.
- `terrain.slangh` — a landscape node's vertex: the grid point morphed by distance, its height and
  normal from the height map.
- `terrain_layers.slangh` — a painted landscape's surface: the paint's weights, up to eight layers
  blended by height, every map read hex-tiled.
- `water.slangh` — the water path, lit by directional light 0 alone: wave normals, fresnel and
  coverage from its depth.
- `elements.slang` — the element pipeline: a rectangle per instance from its vertex index, clipped,
  and a glyph's coverage at its edge.
- `bounce_relight.slang` — the relight's compute: `settle` a probe's validity and moments, `relight`
  its picture into a level's six-axis irradiance, and `sum`.
- `matrix_probe.slang` — writes three elements of a matrix out as colour, so a test can tell which
  layout slangc used.
