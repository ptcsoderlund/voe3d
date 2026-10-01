# 01 — Shader parts rebuild every shader
folder: cmake
after: none
decisions: 0168, 0308

## Change
0308 point 8. Read `cmake/voe.cmake` from `voe_render_shaders()` (around
line 415) to its end, and `cmake/cmake.md`.

- `voe.cmake`, `voe_render_shaders()`: glob `shaders/*.slangh` with
  `CONFIGURE_DEPENDS` beside the `*.slang` glob; every shader's custom
  command lists those parts in `DEPENDS` with its own source, so editing a
  part rebuilds every shader. The parts are not compiled on their own (the
  `*.slang` glob does not match them). slangc finds an `#include "x.slangh"`
  beside the including shader; add the shaders folder to its include path
  only if the command does not already run there.
- The comment block above the function: points that a `.slangh` is a part
  included by shaders, never compiled alone, and that every shader depends on
  every part.
- `cmake/cmake.md`: the `voe.cmake` entry mentions shader parts.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_render`
exits 0, and `grep -n 'slangh' cmake/voe.cmake` shows the glob and the
`DEPENDS` use.
