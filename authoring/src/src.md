# src

`authoring`'s implementation: the writer, the reader and the project file behind
the three public headers, and the two helpers they share. Nothing here opens a
file and nothing here knows what a transform or a camera is — a field is written
and read through the description its own folder registered.

- `scene_write.c` — the writer's walk. Its header says why it walks the world
  twice and how kept sections are merged in.
- `scene_tree.h` — the writer's walk narrowed to one tree, which the prefab
  writer calls; its header says what changes for the root.
- `prefab_write.c` — the prefab writer: refuses a root that cannot head a prefab
  and hands its tree to the walk.
- `value_write.h` — one field's value spelled into the text, and every refusal
  about a value.
- `value_write.c` — the numbers, floats, strings, references and a field's
  nested brackets. Its header says why the split falls at one field.
- `scene_read.c` — the reader's two passes. Its header says why only the second
  touches the world and why it walks the text beside the sectioned reader for
  key spans.
- `field_read.h` — one field's value read from the text into a row, and how an
  entity reference is held until the entity exists.
- `field_read.c` — the tokens, numbers, brackets and strings. Its header says why
  nothing recurses and how numbers are read.
- `scene_cook.c` — the cook. Its header says why a field's dimensions may recurse
  and where the growing text lives.
- `project.c` — the project file reader and writer. Its header says how a scene
  path is checked the same way on both sides.
- `key_span.h` — each key's own trimmed bytes in a sectioned document, which
  `scene_read.c` writes a kept section back out from; a line is the parser's.
- `key_span.c` — the second walk of the same text that fills those spans.
- `authored.h` — the sort and search by authored id both of them use, and why
  neither is the C library's.
- `authored.c` — the merge sort and the binary search.
