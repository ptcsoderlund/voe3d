# src

`authoring`'s implementation: the writer, the reader and the project file, and
the two internal pairs all three share.

- `scene_write.c` — the writer. Its header says why it walks the world twice and
  how kept sections are merged in.
- `scene_read.c` — the reader. Its header says why only its second pass touches
  the world and why it walks the text beside the sectioned reader.
- `project.c` — the project file reader and writer. Its header says how a scene
  path is checked the same way on both sides.
- `line_index.h` — the line number of every section and key in a sectioned
  document, shared by `scene_read.c` and `project.c`, both of which name a line
  in a refusal. Its header says why it is a second walk of the same text.
- `line_index.c` — that walk: the classification of a line, and the counts tied
  to the parsed document's.
- `authored.h` — the sort and search by authored id both of them use, and why
  neither is the C library's.
- `authored.c` — the merge sort and the binary search.
