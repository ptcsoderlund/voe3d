# authoring

`authoring`'s public headers: a world written as scene text, scene text read back
into a world, and the project file. A caller hands over the arena the text lands
in; nothing here opens a file.

- `scene_write.h` — a world written as scene text. Its header says which
  entities, components and kept sections are written and in what order, how every
  field kind is spelled, and each thing it refuses rather than writing.
- `scene_read.h` — scene text read into a world. Its header says why a load may
  create rows, that a file is validated whole first, what a world that runs out
  of room is left as, and what is kept rather than read.
- `project.h` — `project.voe3d` read and written. Its header says the one key it
  holds, what it refuses, and why the writer cannot fail.
