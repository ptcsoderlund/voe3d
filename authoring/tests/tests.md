# tests

One plain C program per `authoring` module, found by the build, each checking
that module's promises from outside. Nothing here opens a file: a test hands the
writer a world it built and the reader text it holds in memory.

- `scene_write.c` — the exact bytes for a small scene, for a component holding
  every kind and for kept sections, fields of every shape from rank 0 to 7, the
  shortest float spellings, every refusal, and the same bytes twice.
- `scene_read.c` — a canonical file read and written back byte for byte, a world
  written and read back row for row, a COLOUR field written as three numbers and
  read back byte for byte, every refusal creating nothing, the warnings that still
  load (a missing field reading as its type's default, or zero), and fields of
  every shape from rank 0 to 7.
- `project.c` — the exact bytes the writer emits, a round trip through the
  reader, every refusal with the line it names, and an unknown key that warns and
  still loads.
