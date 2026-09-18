# tests

One plain C program per module, found by the build, checking the exact bytes on
both sides and every refusal. None of them needs a graphics card.

- `scene_write.c` — the exact bytes for a small scene, for a component holding
  every kind and for kept sections, fields of every shape from rank 0 to 7, the
  shortest float spellings, every refusal, and the same bytes twice.
- `scene_read.c` — a canonical file read and written back byte for byte, a world
  written and read back row for row, every refusal creating nothing, the warnings
  that still load, and fields of every shape from rank 0 to 7.
- `project.c` — the exact bytes the writer emits, a round trip through the
  reader, every refusal with the line it names, and an unknown key that warns and
  still loads.
