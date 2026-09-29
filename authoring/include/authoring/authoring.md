# authoring

The public headers, one entry each; the fuller account of every one of these,
and the questions each header answers, stays on `authoring/authoring.md`.

- `scene_write.h` — a world written out as scene text, handed back in one
  `voe_authoring_text` out-struct.
- `prefab.h` — one tree in a world written as a `.prefab` file's scene text.
- `scene_read.h` — scene text read into a world, the one load that may create
  rows.
- `scene_cook.h` — a world cooked into C source, one function the game compiles in.
- `project.h` — `project.voe3d` read and written, holding the one key that names
  the project's scene.
