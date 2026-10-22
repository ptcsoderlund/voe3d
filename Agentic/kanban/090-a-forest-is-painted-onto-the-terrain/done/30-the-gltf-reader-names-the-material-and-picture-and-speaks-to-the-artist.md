# 30 — The glTF reader names the material and picture and speaks to the artist
folder: assets
after: 29
decisions: 0168, 0443, 0445

## Change
0445 points 1, 2 and 5 for the reader's shared helpers, its top-level checks and its materials and
pictures. Card 31 does the meshes, nodes, container and JSON over the helpers made here.

- `assets/src/model_gltf_read.h` —
  - `struct gltf` gains two subject buffers (one a level: a part, then a material within it), each
    `VOE_ASSETS_MODEL_NAME` plus room for the words, and how many are pushed.
  - `void voe_assets_gltf_subject_push(struct gltf *gltf, const char *kind, const
    voe_assets_model_name *name, uint32_t index)`: composes "the <kind> `name`", or "<kind> <index>"
    with no name, and pushes it on `base/report.h`'s stack; `void voe_assets_gltf_subject_pop(struct
    gltf *gltf)`. A failure returns through the pops, so a pushed subject is popped on every path.
  - `voe_assets_gltf_malformed` becomes printf-style over the format's words; its line is the
    damaged-file sentence (export it again from Blender) then `(glTF: <words>)`.
  - `voe_assets_gltf_unsupported` becomes `(struct gltf *gltf, const char *format_words, const
    char *artist, ...)`, printf over `artist`: the artist's sentence with its numbers and what to do
    in Blender, then `(glTF: <words>)`.
  - The header says the two shapes and that a subject is pushed by the part that reads a thing.
  - The one `_unsupported` call in `model_gltf_meshes.c` takes the new shape, with its sentence.
- `assets/src/model_gltf.c` — the helpers' bodies; `voe_assets_gltf_uint_member`'s refusal names the
  member, the value and the limit in the format's words; a required extension names it and what in
  Blender's glTF export writes it (Draco is Compression; others: untick what adds it); a version
  other than 2 says to export with Blender's own glTF exporter. Header: the line's shape.
- `assets/src/model_gltf_materials.c` — each material read under "material" pushed, each image
  decoded under "picture" (its glTF name, else its index); every refusal in the artist's terms:
  another UV set than the first (which UV map, the first only, make it the first in Blender's UV
  Maps); a picture in a separate file or a data URI (export as glTF Binary, `.glb`); a picture
  neither PNG nor JPEG (set Images to PNG or JPEG in the export); a picture of no bytes or past the
  chunk (damaged). A decoder's refusal is reported by the decoder under the pushed picture, no
  second line here.
- `assets/include/assets/model.h` — the header's refused paragraph: every refusal is one line naming
  the part, material or picture, in the artist's words, the format's after it (0443, 0445).
- `assets/src/src.md` — the `model_gltf_read.h` entry names the subjects and the two report shapes.
- `assets/tests/model.c` (glb builder in `model_data.inc` if there) — new checks on
  `voe_base_report_error_first()` after a clear: a file requiring `KHR_draco_mesh_compression`
  names it and contains `(glTF: `; a material `leaf` reading `texCoord` 1 begins "the material
  `leaf`: " and says "UV map"; a texture index past the textures begins with its material's subject
  and says to export again. Existing checks of old wording in this folder are updated.

## Done when
`ctest --test-dir build/debug -R '^assets/model$'` passes with the new checks.
