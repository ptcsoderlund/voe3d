# shaders

The engine's shaders, and the only files `slangc` is run over. Slang source,
compiled to SPIR-V at build time and `#embed`ded in the binary: nothing is read
from disk at run time and a shipped build carries no shader source. `slangc` is
invoked with `-matrix-layout-row-major`, the layout the engine stores matrices
in; getting it wrong transposes every transform without failing to compile.

A `.slangh` is a part, included by a shader and never compiled alone.

- `bindings.slangh` — the part `draw.slang` includes first: the frame block with its bounce record
  and point light count, object and shading records, the probe volumes' sums at binding 6 and
  moments at 10, the point lights and their bins at 7 and 8, the point shadow maps at 9, set 0's
  bindings and the push constant, which holds the object and a 96-bit face mask, each matching its C
  struct.
- `draw.slang` — the mesh pipelines' entry points, the only place a matrix is
  applied to a position, the three numbers a draw finds everything by, the
  normal map, the distance field, the alpha modes and three unlit exits, the
  third for a pass with no sun; its lit exit adds the point lights; the point-shadow vertex
  stage, one instance per face writing its layer; and the capture pass's two, albedo and normal
  with distance from the probe, out to twelve of the volume's cells.
- `point_shadow.slangh` — a point light's cube faces: the face of a vector by its major axis and the
  90° reversed-depth clip position on a face, near 0.05 m, far the light's range; draw and lookup
  share it.
- `bounce_read.slangh` — the bounce's one read, E(n) from a probe volume's six-axis irradiance:
  eight probes weighted by trilinear, validity, facing and Chebyshev visibility, normalised, faded
  at the edge (0326).
- `lighting.slangh` — the sun's light: glTF's metalness-roughness BRDF terms, its shadow by
  cascades, its radiance, the bounce read from the pass's probe volume with no gain (0317), the
  fill, a floor under the bounce (0307 amends 0275), and the binned point lights (0320), each fading
  to its range by its falloff (0322), a slotted one shadowed by one compare on its cube face (0325).
- `water.slangh` — the water path: wave normals, fresnel to the sky and
  coverage from its thickness over the pass's depth copy.
- `elements.slang` — the element pipeline's two entry points: a rectangle per
  instance built from a vertex index, a clip test, and a glyph's coverage across
  one pixel around an edge that moves out for a thin stroke, not a wide one. It
  copies the field's spread, the face's thinnest stroke and the median.
- `bounce_relight.slang` — the relight's compute: `settle`, a listed probe's validity, nought with
  no picture or over a quarter of its texels on back faces, and each texel's distance moments over
  its 3×3; `relight`, a probe's picture, no hit past twelve cells, lit by a chain's lights or the
  level below into a level's six-axis irradiance (0327); `sum` (0326).
- `matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
