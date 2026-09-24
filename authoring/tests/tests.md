# tests

One plain C program per `authoring` module, found by the build, each checking
that module's promises from outside. Only the cook's compile check writes a
file; the rest hand the writer a world and the reader text held in memory.

- `scene_write.c` — the exact bytes for a small scene, for a component holding
  every kind and for kept sections, fields of every shape from rank 0 to 7, the
  shortest float spellings, every refusal, and the same bytes twice.
- `scene_read.c` — a canonical file read and written back byte for byte, a world round-tripped row
  for row, every refusal creating nothing, the warnings that still load, and fields of every shape
  from rank 0 to 7.
- `scene_cook.c` — the exact source for a small scene, that source compiled by
  `clang`, the refusals leaving `*out` untouched, and an empty world.
- `project.c` — the exact bytes the writer emits, a round trip through the
  reader, every refusal with the line it names, and an unknown key that warns and
  still loads.
