# 10 — The sky builds in Release
folder: render
after: none
decisions: 0168

## Change
The whole-suite Release build (step 3b of `check.cmake`) fails: in `render/src/sky.c`
the static helper `unit_error` is called only inside `assert`s, so at `-DNDEBUG` it is
unused and `-Werror,-Wunused-function` stops the build.

In `render/src/sky.c` only: mark `unit_error` `[[maybe_unused]]`, the pattern already
used for assert-only helpers in `render/src/element.c` (`range_names`) and
`render/src/draw.c` (`shading_names`). Change nothing else.

## Done when
`cmake -S render -B build/sky-release -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release && cmake --build build/sky-release --target voe_render`
exits 0, then `rm -rf build/sky-release`.
