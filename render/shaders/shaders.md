# shaders

The engine's shaders, and the only files `slangc` is run over. Slang source,
compiled to SPIR-V at build time and `#embed`ded in the binary: nothing is read
from disk at run time and a shipped build carries no shader source. `slangc` is
invoked with `-matrix-layout-row-major`, the layout the engine stores matrices
in; getting it wrong transposes every transform without failing to compile.

A `.slangh` is a part, included by a shader and never compiled alone.

- `bindings.slangh` — the part `draw.slang` includes first: the frame block
  with its bounce record, object and shading records, the bounce grids at binding 6, set 0's bindings and the push constant, each
  matching its C struct.
- `draw.slang` — the mesh pipelines' entry points, the bounce map's fragment
  among them, the only place a matrix is
  applied to a position, the three numbers a draw finds everything by, the
  normal map, the distance field, the alpha modes and three unlit exits, the
  third for a pass with no sun; it includes the three parts.
- `lighting.slangh` — the sun's light: glTF's metalness-roughness BRDF terms,
  its shadow by cascades, its radiance and the fill that lifts its shade.
- `water.slangh` — the water path: wave normals, fresnel to the sky and
  coverage from its thickness over the pass's depth copy.
- `elements.slang` — the element pipeline's two entry points: a rectangle per
  instance built from a vertex index, a clip test, and a glyph's coverage across
  one pixel around an edge that moves out for a thin stroke, not a wide one. It
  copies the field's spread, the face's thinnest stroke and the median.
- `bounce.slang` — the bounce grid update's two compute entries: `reduce`, the
  bounce map to 64×64 virtual point lights, and `gather`, the listed probes'
  L1 SH from them, blended into a grid's three images.
- `matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
