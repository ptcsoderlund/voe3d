# 26 — The dev program builds its quad, cubes and monitor white again
folder: dev
after: 23, 24, 25
decisions: 0168, 0421, 0422

## Change
Card 07 gave `voe_render_vertex` a fourth field, `colour_complement` (`uint16_t[4]`, zeroed is
white). clang's `-Wmissing-field-initializers` now rejects every positional
`{ {pos}, {normal}, {uv} }` built in this folder, so it does not compile. 0422: each such initializer
gains a trailing `{ 0 }`, nothing else; what is drawn does not change. A designated initializer or a
vertex filled field by field is left alone. No header, `.md` or test changes.

Files: `dev/src/quad.c`, `dev/src/cubes.c`, `dev/src/monitor.c`.

Find the initializers by building the target below and reading each `missing field
'colour_complement'` error; clang stops at 20 errors per file, so build again until none is left.

## Done when
`cmake --build --preset debug --target voe_dev` exits 0 (warnings are errors).
