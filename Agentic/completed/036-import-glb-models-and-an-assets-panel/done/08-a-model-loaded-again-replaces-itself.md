# 08 — A model loaded again replaces itself
folder: 3d
decisions: 0168, 0277, 0278

## Change
Needs cards 02 and 07. A re-export and a project change (0277 point 5).

- `3d/include/3d/models.h` and `3d/src/models.c`:
  - `voe_3d_models_load` on a path already held: success replaces the entry's parts and shape,
    then frees the old geometries (`voe_render_geometry_destroy`), shading records
    (`voe_render_shading_destroy`) and textures (`voe_render_texture_destroy`) and the old
    entry's memory; failure keeps the old parts and `loaded` as they were, sets the new stamp,
    and returns false with the category. A failed entry loaded again with good bytes becomes
    loaded. Remove the assert card 07 put there; rewrite that paragraph.
  - `void voe_3d_models_clear(voe_3d_models *, voe_render_device *);` — frees every entry's GPU
    parts and memory; the store is empty and reusable. `_destroy`'s paragraph says a program that
    keeps its device calls `_clear` first.
  - Header point: the store is changed only between frames, since every free waits for idle.
- `3d/tests/models.c`: cases — the same path loaded 100 times on a device sized for three copies
  of it still loads, one entry; ids change between loads; good then bad bytes keeps the entry
  loaded with the new stamp; bad then good makes it loaded; `clear` leaves `find` NULL and a load
  after it works. Update its line in `3d/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0.
