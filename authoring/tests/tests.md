# tests

One plain C program per `authoring` module, found by the build, each checking
that module's promises from outside. Only the cook's compile check writes a
file; the rest hand the writer a world and the reader text held in memory.

- `scene_write.c` — the exact bytes for a small scene, for a component holding
  every kind and for kept sections, fields of every shape from rank 0 to 7, the
  shortest float spellings, every refusal, the same bytes twice, and a placed
  copy written as its root alone.
- `prefab.c` — the prefab writer's exact bytes for a placed three-deep tree,
  the reader putting that text onto a placed root, the cook turning a hull and
  turret into a spawning function, and each one's refusals, a read without
  room for its identities among them.
- `scene_read.c` — a canonical file read and written back byte for byte, a world
  round-tripped row for row, every refusal creating nothing, the warnings that
  still load, and fields of every shape from rank 0 to 7.
- `scene_read_unsaid.c` — a field a section does not mention read from the
  type's unsaid row, else its default, a said field kept, and a light with no
  `cast_shadows` reading true, and an 047 light keeping bounces 1 at strength 1.
- `scene_read_former.c` — a blocker saved with `kind` read as its `block`,
  written back as `block` alone, `block` winning over `kind` in either order,
  and a blocker saying neither reading All.
- `scene_cook.c` — the exact source for a small scene, the sun's bounces,
  `cast_shadows` and `bounce_strength` included, that source compiled by `clang`, the refusals leaving
  `*out` untouched, and an empty world.
- `project.c` — the exact bytes the writer emits, a round trip through the
  reader, every refusal with the line it names, and an unknown key that warns and
  still loads.
