# shaders

The engine's shaders, and the only files `slangc` is run over. Slang source,
compiled to SPIR-V at build time and `#embed`ded in the binary: nothing is read
from disk at run time and a shipped build carries no shader source. `slangc` is
invoked with `-matrix-layout-row-major`, the layout the engine stores matrices
in; getting it wrong transposes every transform without failing to compile.

- `draw.slang` — the two pipelines' two entry points, the only place a matrix is
  applied to a position, the three numbers a draw finds everything by, and the
  whole of this engine's lighting: one sun, its shadows by cascades, glTF's
  metalness-roughness BRDF, three unlit exits, the third for a pass with no sun,
  and water: wave normals faded over the pass's depth copy.
- `elements.slang` — the element pipeline's two entry points: a rectangle per
  instance built from a vertex index, a clip test, and a glyph's coverage across
  one pixel around an edge that moves out for a thin stroke, not a wide one. It
  copies the field's spread, the face's thinnest stroke and the median.
- `matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
