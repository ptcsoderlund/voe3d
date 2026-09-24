# 02 — authoring cooks a world into C source
folder: authoring
decisions: 0168, 0236, 0237

## Change
The one cook (0235, 0237 point 1). Nothing calls it yet (card 06).

- `authoring/include/authoring/scene_cook.h` — new.
  - `[[nodiscard]] bool voe_authoring_scene_cook(const voe_ecs_world *world, const char *include,
    const char *function, voe_base_arena *arena, voe_authoring_text *out);` Same arena and failure
    contract as `voe_authoring_scene_write` (read `scene_write.h`'s header; reuse its text type).
  - The text: `#include <include>`, then `bool <function>(voe_ecs_world *world)` that creates one
    entity per entity with an identity, ascending by authored id, into a local array; then, entity by
    entity and type by type in key-name byte order (as the writer orders them), one
    `voe_ecs_component_add(world, voe_ecs_component_type(world, &<key>_key), e[i], &(<key>){ … })`
    per row, returning false at the first refusal and true at the end. A world with no authored
    entity cooks a function that returns true.
  - Which types: every described type that is not runtime-only, identity included. `<key>` is the
    key's name, which is also the struct's name.
  - Values, by field kind and shape, as designated initialisers `.name = …`: integers in decimal
    with the suffix their width needs; FLOAT32/FLOAT64 as `%a` hex literals (`f` suffix for 32);
    BOOL `true`/`false`; ENUM as its integer; vector kinds, FLOAT4X4 and each array dimension as
    nested braces in memory order; CHAR as a string literal up to the first NUL with `"`, `\` and
    bytes outside printable ASCII escaped (octal); ENTITY as the cooked entity with that authored id,
    or a zeroed `voe_ecs_entity` for a dead one or one with no identity.
  - Refusals, each reported naming entity, component and field: a NaN or infinite float, two
    entities with one authored id, a described type whose description this build compiled out.
  - Header points: C source because the game compiles its data in and never parses project text
    (0236); hex floats because Play must show the editor's exact bytes (0188); the cooked function
    is rule 3's third creation exception (0237); kept sections are not cooked (nothing reads them).
- `authoring/src/scene_cook.c` — new. Reuses `authoring/src/authored.h` for the sort and the
  find-by-id; read that header and `scene_write.h`'s, no other file.
- `authoring/tests/scene_cook.c` — new, on a world registering scene's transform, identity, light
  and camera (as `authoring/tests/scene_write.c` registers them; read only its setup):
  - `cook_exact_text`: two entities out of id order, a transform with a `0.1f` and a `-0.0f`, a
    light, and a camera — the whole text compared with an expected literal.
  - `cook_compiles`: the text for that world written with stdio into the working directory beside a
    header that includes scene's four component headers, then `system()` runs
    `clang -std=c23 -fsyntax-only -Wall -Wextra -Wpedantic -Werror -DVOE_BASE_DESCRIPTIONS=1` with
    `-I` for `base`, `math`, `ecs` and `scene`'s include folders found from `__FILE__`; exit 0.
  - `cook_refuses_nan` and `cook_refuses_duplicate_id`: false, `*out` untouched.
  - `cook_empty_world`: a function that returns true.
- `authoring/include/authoring/authoring.md`, `authoring/src/src.md`, `authoring/tests/tests.md` — an
  entry each; `authoring/authoring.md` says the folder also cooks.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `authoring` exits 0, and `ctest --test-dir
build/debug -R '^authoring/scene_cook$'` passes.
