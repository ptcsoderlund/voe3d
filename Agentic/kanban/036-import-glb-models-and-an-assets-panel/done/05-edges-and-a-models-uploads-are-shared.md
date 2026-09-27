# 05 — Edges and a model's uploads are shared
folder: 3d
decisions: 0168, 0277, 0202

## Change
Two moves with no change in behaviour, so the store of card 07 reuses them (0277 point 2).

- `3d/include/3d/shape_geometry.h`: add
  `void voe_3d_shape_geometry_build(voe_base_arena *arena, const voe_render_vertex *vertices,
  uint32_t vertex_count, const uint32_t *indices, uint32_t index_count,
  voe_3d_shape_geometry *out);` — points `out` at the caller's arrays (which must outlive it)
  and finds the welded edges into `arena`. The header says it is how any triangles, not only the
  three shapes, get what a ray and a silhouette need.
- `3d/src/shape_geometry.c`: `_build` is the existing weld and `build_edges` walk;
  `voe_3d_shape_geometries_create` calls it for each kind.
- New internal `3d/src/model_upload.h` and `3d/src/model_upload.c`: what `3d/src/import.c` does
  today in `upload_images`, `texture_at`, `alpha_mode_of`, `upload_materials` and
  `default_material`, moved behind one call over a read `voe_assets_model`: every picture
  uploaded once per colour space it is wanted in, one uploaded `voe_3d_material` per glTF
  material plus the default one for a primitive with none, all into a result struct in the
  caller's arena that also lists every texture id and every shading id it made (the store frees
  them on a reload). Headers say what moved and why internal.
- `3d/src/import.c`: calls it; its header's paragraphs on colour spaces point to
  `model_upload.h`. `voe_3d_import_glb`'s behaviour and counts are unchanged.
- `3d/src/src.md`: lines for the two new files; the `import.c` and `shape_geometry.c` lines.
- `3d/tests/shape_geometry.c`: a case — the cube kind's own arrays through `_build` give the same
  edge count as the cube kind. Update its line in `3d/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `3d/import` and `3d/shape_geometry` among them.
