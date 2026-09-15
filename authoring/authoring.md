# authoring

A world turned into a scene file, and a scene file turned back into a world
(ADR-0151). Scene text is the format ADR-0149 names; what goes into it is what a
person authored, ADR-0150.

**It needs the field descriptions, and a shipped game does not link it.** Nothing
here knows what a transform or a camera is: a component is written field by field
through the description its folder registered, and a game's own build compiles
those out (ADR-0145). So this folder is authoring-time code — the editor's, and a
tool's — and a game that turns descriptions on, to load scenes at run time, may
link it.

The writer reads components and writes none. The reader is the one exception to a
component being written only by its own system: it adds rows to entities it has
just created, and edits nothing (ADR-0152). Nothing here opens a file: `platform`
owns files, and text lands in an arena the caller hands over.

- `include/authoring/scene_write.h` — a world written as scene text. Its header
  says which entities, components and kept sections are written and in what order,
  how every field kind is spelled, and each thing it refuses rather than writing.
- `include/authoring/scene_read.h` — scene text read into a world. Its header says
  why a load may create rows, that a file is validated whole first, what a world
  that runs out of room is left as, and what is kept rather than read.
- `src/scene_write.c` — the writer. Its header says why it walks the world twice
  and how kept sections are merged in.
- `src/scene_read.c` — the reader. Its header says why only its second pass touches
  the world and why it walks the text beside the sectioned reader.
- `src/authored.h` — the sort and search by authored id both of them use, and why
  neither is the C library's.
- `src/authored.c` — the merge sort and the binary search.
- `tests/scene_write.c` — the exact bytes for a small scene, for a component
  holding every kind and for kept sections, fields of every shape from rank 0
  to 7, the shortest float spellings, every refusal, and the same bytes twice.
- `tests/scene_read.c` — a canonical file read and written back byte for byte, a
  world written and read back row for row, every refusal creating nothing, the
  warnings that still load, and fields of every shape from rank 0 to 7.
