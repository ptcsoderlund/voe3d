# shaders

The engine's shaders, and the only files `slangc` is run over. Slang source,
compiled to SPIR-V at build time and `#embed`ded in the binary: nothing here is
read from disk at run time and a shipped build carries no shader source.

`slangc` is invoked with `-matrix-layout-row-major`, because that is the layout
the whole engine stores matrices in and an upload to the GPU is a straight copy.
Its default is the other one, and getting it wrong transposes every transform
without failing to compile — which is why one of these three files exists only
to prove which layout was used.

- `draw.slang` — the two pipelines' two entry points, the only place a
  matrix is applied to a position, the three numbers a draw finds everything by,
  and the whole of this engine's lighting. Its header says why the only push
  constant left is an object's number, and it is where every decision in the
  shading is written down: one sun and no shadows, glTF's metalness-roughness
  BRDF, what a surface with no normal looks like, why occlusion is applied where
  glTF does not put it, and which two different things take the same unlit exit
  out of the fragment stage.
- `elements.slang` — the element pipeline's two entry points: four
  corners built out of a vertex index, one instance per rectangle, a clip test in
  the fragment stage, and the sheet a glyph reads its coverage out of. Its header
  says why it is a second shader rather than a fourth branch in `draw.slang`, why
  nothing is read from a vertex buffer, why four vertices and a strip rather than
  six, why the record's number takes two semantics added together and what
  reading only one of them looks like, that order is paint order and what relies
  on it, why its push constant
  aliases `draw.slang`'s, what the image kind multiplies and why its picture's alpha is straight, why
  the threshold is a threshold and not a smoothstep,
  why the sheet is sampled at an explicit level, why its texture index is
  non-uniform where `draw.slang`'s is not, and that it holds a second copy of the
  median.
- `matrix_probe.slang` — reads a matrix and writes three of its elements
  out as colour, so that a test can tell which layout slangc used.
