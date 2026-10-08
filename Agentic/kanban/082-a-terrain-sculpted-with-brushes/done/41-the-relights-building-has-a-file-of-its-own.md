# 41 — The relight's building has a file of its own
folder: render/src
after: none
decisions: 0168, 0389

## Change
`render/src/bounce_relight.c` is 740 lines, and the next cards add to it. Split it by function, with no
change in behaviour:

- New `render/src/bounce_relight_build.c`: the embedded SPIR-V, the push block and its static asserts,
  `create_layouts`, `create_pipelines`, `build_mapped`, `record_stride`, `create_sets_and_buffers`,
  `voe_render_bounce_relight_startup` and `voe_render_bounce_relight_shutdown`. Its header comment covers the
  set layout, the per-slot list and record buffers, and the record stride, moved from `bounce_relight.c`'s.
- `render/src/bounce_relight.c` keeps `voe_render_bounce_begun_lights`, listing, the record, the set write,
  the barriers, the dispatches and `voe_render_bounce_relight`. Its header keeps the order of one relight and
  the constraints, and points to the new file for what is built.
- What both files need (the push block's layout, `volume_count`, the holds bit) goes in a new internal
  `render/src/bounce_relight.h` with a short header comment. Nothing outside these files changes.
- `render/CMakeLists.txt`: add the new source beside the old one.
- `render/src/src.md`: an entry for each new file; the `bounce_relight.c` entry shrinks to what it keeps.

## Done when
`wc -l render/src/bounce_relight.c` prints under 500. `grep -c "create_pipelines" render/src/bounce_relight.c`
prints 0. `ctest --test-dir build/debug -R '^render/bounce_'` passes after a build.
