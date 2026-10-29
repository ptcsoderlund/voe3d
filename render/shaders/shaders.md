# shaders

The engine's shaders and the only files `slangc` runs over: Slang compiled to row-major SPIR-V at
build time and `#embed`ded, so nothing is read at run time. A `.slangh` is a part, included by a
shader and never compiled alone.

- `bindings.slangh` — the part `draw.slang` includes first: the frame block with its clouds, the
  object and instance records, set 0's bindings with the cloud field at 13 and 14, and the push
  constant, each matching its C side; an object carries paint, layers, dither, instances, a ground.
- `instances.slangh` — where a mesh draw's copy stands: from the record and the instance index its
  world matrix, normal matrix and dither, the record's own when it names no instances.
- `draw.slang` — the mesh pipelines' nine entry points, the capture's, the sky's and the clouds' in
  its parts below: placing, maps, a painted landscape's layers, a ground record's blend, alpha and
  dither, the lit and unlit exits hazed, exposed and tone mapped, and the point-shadow vertex stage.
- `draw_capture.slangh` — the part `draw.slang` includes at its end: the bounce capture pass's two
  entry points, a mesh on a probe's faces as albedo and normal with distance.
- `draw_sky.slangh` — the part `draw.slang` includes last: the sky's two full-screen entry points,
  the sky or the clear colour veiled by the haze.
- `draw_clouds.slangh` — the part `draw.slang` includes after `draw_sky.slangh`: the clouds' march
  at half size to the furthest depth, veiled by the haze, and their bilinear composite, skipped
  where a surface is nearer than the layer, both behind the sky's vertex stage.
- `tone.slangh` — haze, exposure and tone at every mesh exit: mixed by distance past the haze start
  towards the drawn sky's horizon hue at the lights' brightness, or else their colour, × 2^exposure,
  Clip or Soft; a `plain` draw as authored; the veil along a view below the haze's top, endless or
  stopping at a distance.
- `sky.slangh` — whether a light is shown, the sky's backdrop and its radiance along a view, that
  backdrop plus each of the frame's shown lights' glow then disc, twin of `src/sky.h`.
- `clouds.slangh` — the cloud field read by the shaders: density, where a view meets the layer,
  a light's transmittance, the light a unit of cloud sends, the march and a reflection's sample,
  all one field, in eye-relative metres.
- `point_shadow.slangh` — a point light's cube faces: the face of a vector and the clip position on
  it; draw and lookup share it.
- `blockers.slangh` — which light blockers hold a point and whether a segment passes them, twin of
  `src/light_blockers.c`.
- `bounce_read.slangh` — the bounce's one read: E(n) from a probe volume's eight nearest probes,
  weighted and normalised.
- `lighting.slangh` — the directional lights, their shadows and fills, the bounce and the binned
  point lights, each gated by the light blockers; the sky lights nothing, and a metal mirrors it
  only as bright as the light on it.
- `terrain.slangh` — a landscape node's vertex: the grid point morphed by distance, its height and
  normal from the height map.
- `terrain_layers.slangh` — a painted landscape's surface: the paint's weights, up to eight layers
  blended by height, every map read hex-tiled, by the uv's own gradients or given ones.
- `ground_blend.slangh` — a ground record's model blending into the painted ground under it:
  weight by height over the ground within the reach, colour, ORM and normal lerped to the ground's.
- `water.slangh` — the water path, lit by directional light 0 alone: wave normals, fresnel and
  coverage from its depth.
- `elements.slang` — the element pipeline: a rectangle per instance from its vertex index, clipped,
  and a glyph's coverage at its edge.
- `bounce_relight.slang` — the relight's compute: `settle` a probe's validity and moments, `relight`
  its picture into a level's six-axis irradiance, and `sum`.
- `compress.slang` — the texture encoder's compute: `bc7` and `bc5`, one thread a 4×4 block of a
  level, ports of the assets CPU encoders block for block.
- `matrix_probe.slang` — writes three elements of a matrix out as colour, so a test can tell which
  layout slangc used.
