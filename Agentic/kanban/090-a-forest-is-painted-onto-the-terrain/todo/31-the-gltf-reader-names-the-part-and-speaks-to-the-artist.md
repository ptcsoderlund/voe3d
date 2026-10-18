# 31 — The glTF reader names the part and speaks to the artist
folder: assets
after: 30
decisions: 0168, 0443, 0445

## Change
0445 points 1, 2 and 5 for the rest of the glTF read, over card 30's helpers and subjects
(`assets/src/model_gltf_read.h`; read its header first).

- `assets/src/model_gltf_meshes.c` — each mesh read under the subject "part" (its glTF name, else
  its index), each primitive under "material" (its material's name, else "no material"). Every
  refusal through the helpers, in the artist's terms:
  - a mode other than triangles: the part has points or lines, the engine draws triangles; in
    Blender delete loose edges and points, or Triangulate;
  - a sparse accessor: shape keys or morph targets; untick Shape Keys in the export;
  - a normalized or quantized attribute, or a COLOR_0 of a type not read: which attribute and how
    it is stored, what the engine reads, untick Compression or Quantize in the export;
  - a missing POSITION, unequal attribute counts, an index past the part's vertices, an index count
    not a multiple of three, and every other broken accessor: the damaged-file shape.
- `assets/src/model_gltf_nodes.c` — each node checked under "object" (its name, else its index);
  every refusal is the damaged-file shape, but nesting past the reader's depth, a limit: how deep
  it nests, the most the engine takes, flatten the hierarchy in Blender (Clear Parent and Keep
  Transform).
- `assets/src/model_glb.c` — its lines through the same two shapes, with `(glTF: …)`: a `.gltf`
  says export as glTF Binary (`.glb`); a container not version 2 says export with Blender's glTF
  exporter; every truncated or misshapen chunk is the damaged-file shape. One line per refusal.
- `assets/src/json.h`, `assets/src/json.c` — a syntax error is the damaged-file shape with the
  byte; more values than the reader holds and nesting past its depth are limits, each with the
  file's number and the most the engine reads. json.c has no `struct gltf`: it reports its own
  line in the same shape (the damaged-file sentence or the limit, then `(glTF: …)`), and the glTF
  reader adds no second line over it.
- `assets/src/src.md` — the entries of these four files, where they name what is refused.
- `assets/tests/model.c` (glb builder in `model_data.inc` if there) — new checks on
  `voe_base_report_error_first()` after a clear: a mesh `twig` whose primitive is mode 0 begins
  "the part `twig`: the material " and says "triangles"; a glb cut short says to export again and
  contains `(glTF: `; JSON text handed in as a `.glb` says ".glb". Existing checks of old wording
  here and in `assets/tests/json.c` are updated.
- `assets/tests/tests.md` — the `model.c` entry names the wording checks.

## Done when
`ctest --test-dir build/debug -R '^assets/(model|json)$'` passes with the new checks.
