# src

`authoring`'s implementation: the writer, the reader and the project file behind
the three public headers, and the two helpers they share. Nothing here opens a
file and nothing here knows what a transform or a camera is — a field is written
and read through the description its own folder registered.

- `scene_write.c` — the writer. Its header says why it walks the world twice and
  how kept sections are merged in.
- `scene_read.c` — the reader. Its header says why only its second pass touches
  the world and why it walks the text beside the sectioned reader.
- `project.c` — the project file reader and writer. Its header says how a scene
  path is checked the same way on both sides.
- `line_index.h` — the line number of every section and key in a sectioned
  document, shared by `scene_read.c` and `project.c`, both of which name a line
  in a refusal.
- `line_index.c` — the second walk of the same text that fills that index, and
  each key's own trimmed bytes with it.
- `authored.h` — the sort and search by authored id both of them use, and why
  neither is the C library's.
- `authored.c` — the merge sort and the binary search.
