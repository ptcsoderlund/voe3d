# base

Memory, containers, strings and assert — the primitives any other folder may
depend on. Not maths, and nothing that knows an operating system.

- `include/base/assert.h` — the two assert macros. Its header says which one
  survives a release build, why it is that one, and where the line between an
  assert and a recoverable failure runs.
- `include/base/arena.h` — the arena the engine's working memory comes from.
  Its header carries the one thing a caller can get wrong: two pushes are not
  guaranteed to be next to each other.
- `include/base/error.h` — the recoverable-failure codes, and the two shapes a
  failing function takes. Its header says why the codes are categories and never
  incidents.
- `include/base/version.h` — the placeholder that proves the folder builds.
- `src/assert.c` — the one function both macros expand to.
- `src/arena.c` — the blocks behind the arena, and the pointer bump.
- `src/error.c` — one phrase per code.
- `src/version.c` — the placeholder's one function, and the C23 assertion.
- `tests/arena.c` — the arena's promises, checked from outside.
