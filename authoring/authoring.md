# authoring

A world turned into a scene file, and — when the reader lands — a scene file
turned back into a world (ADR-0151). Scene text is the format ADR-0149 names; what
goes into it is what a person authored, ADR-0150.

**It needs the field descriptions, and a shipped game does not link it.** Nothing
here knows what a transform or a camera is: a component is written field by field
through the description its folder registered, and a game's own build compiles
those out (ADR-0145). So this folder is authoring-time code — the editor's, and a
tool's — and a game that turns descriptions on, to load scenes at run time, may
link it.

It reads components and writes none of them. Nothing here opens a file:
`platform` owns files, and text lands in an arena the caller hands over.

- `include/authoring/scene_write.h` — a world written as scene text. Its header
  says which entities and components are written and in what order, how every
  field kind is spelled, and each thing it refuses rather than writing.
- `src/scene_write.c` — the writer. Its header says why it walks the world twice
  and why neither sort is qsort.
- `tests/scene_write.c` — the exact bytes for a small scene and for a component
  holding every kind, the shortest float spellings read back to the same bits,
  every refusal, and the same bytes twice. Its header says why the every-kind
  description is written by hand.
