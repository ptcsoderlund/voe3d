# 071 — `authoring` reads a scene

claimed-by: claude-opus-5 (session 01TYrewyPCZyspC4KzToTd3K)
blocked-by: 068, 070
decision: *Loading a scene is creation, and the reader adds rows directly* (ADR-0152) every point; the text it reads is ADR-0149 sections 1–4, the value table and points 7–10.

## Goal

`authoring` turns ADR-0149's scene text into entities in a world. A file that is wrong anywhere
creates nothing. Sections naming a component this program never registered are kept and written
back by the writer, so `read` then `write` of a canonical file gives the same bytes.

## Scope

**1. `authoring/include/authoring/scene_read.h`.**

```c
typedef struct { ... } voe_authoring_kept;   // the sections read and not understood

[[nodiscard]] bool voe_authoring_scene_read(const char *text, size_t size,
					    voe_ecs_world *world,
					    voe_base_arena *arena,
					    voe_authoring_kept *out_kept);
```

- `voe_authoring_kept` holds, in the arena, each kept section's authored id, key name, and its
  `key = value` lines as text, in file order. Shape it as the smallest thing the writer needs.
- The world must hold **no entity with an identity**; one that does asserts (ADR-0152 point 7).
- Header paragraphs: that a load creates and is the one sanctioned generic writer (ADR-0152
  point 2, and what it may not do); that a file is validated whole before anything is created;
  that a world that runs out of room mid-creation returns false and must be discarded; that
  loaded values are not settled by any drain (point 6); that the kept sections live in `arena`
  and must outlive the save that writes them back.

**2. What it does**, in two passes.

*Pass one — no world touched.* `voe_assets_sectioned_parse` the text, then for every section:
- `[N]`: `N` a decimal ≥ 1. Its keys are the identity's fields other than `id`.
- `[N.<key>]`: `N` has an `[N]` somewhere in the file. `<key>` found by comparing
  `voe_ecs_component_key(world, type)->name` across the world's types.
  - Registered and described → each field's text read into scratch bytes of the type's size,
    zeroed first. Registered and runtime-only → **refuse**: the file is claiming to hold
    something that is never authored.
  - Not registered → kept, not read.
- Any other section name → refuse.
- **Values**, by kind: exactly ADR-0149's table. `ENTITY` fields are read as authored ids and
  held for pass two. `ENUM` → refuse, message naming the missing value names.
- Missing described field → zero, **warning**. Key naming no field → **warning**, ignored.
- Every refusal reports the line number and returns false with the world untouched.

*Pass two.* Create one entity per `[N]` in file order; build the authored-id → entity map; give
each its identity (`id = N`, the name); patch every held `ENTITY` field through the map — an id
not in the file is no entity and a **warning** (point 5); `voe_ecs_component_add` each row. A
false from an entity create or add returns false, with the header's discard rule.

**3. The writer puts kept sections back.** `voe_authoring_scene_write` in
`authoring/include/authoring/scene_write.h` gains a `const voe_authoring_kept *kept` parameter,
NULL for none. Each kept section is written under its entity, in its sorted place among that
entity's component sections, its lines verbatim and in their original order. A kept section
whose entity no longer has an identity is dropped with a warning. Update card 070's tests for
the new parameter.

**4. The rule's wording.**
- `voe3d/CLAUDE.md` rule 3: one sentence — the exception is a folder's typed creation call and
  `authoring`'s scene reader, both creating and neither editing (ADR-0152). Keep the file under
  its ceiling.
- `ecs/include/ecs/component.h`, the *A COMPONENT IS READ BY ANYONE AND WRITTEN BY ONE* paragraph:
  the same exception, and that `_add` being public does not widen it.

**5. `authoring/tests/scene_read.c`**, no graphics card.
- **Round trip, text first**: a canonical file with three entities, a transform, a camera, an
  entity reference and a kept section of an unregistered type — read, write, bytes identical.
- **Round trip, world first**: build a world, write it, read into a new world, compare every row
  of every described type byte for byte, with references mapped.
- Refusals, each leaving the world with zero entities: a bad float, `bool = 1`, a vector with two
  numbers, `"C:\x"`, a string one byte too long, `[0]`, `[x]`, `[4.voe_scene_transform]` with no
  `[4]`, a runtime-only type's section, an `ENUM` field.
- Warnings, each still loading: a missing field is zero; an unknown key is ignored; a reference to
  an absent id is no entity.
- File order `[3]`, `[1]`, `[2]` loads, and writes back sorted.
- Fixed arrays of a vector kind (`[[0, 0, 0], [1, 1, 1]]`) and a wrong count in one refuse: no
  real component can declare one, since `describe.h` repeats only `ENUM`, `CHAR` and `ENTITY` —
  use a hand-written description, as card 070's every-kind test does. `describe.h` is not edited.
- A world already holding an authored entity asserts — test it only if `voe::testing` can catch
  an assert; otherwise say so in Notes.

**6. `authoring/authoring.md`** — the two new files and the exception.

## What must not change

- No `voe_ecs_component_set`, no remove, no intent submitted from `authoring`.
- `ecs`, `scene`, `assets` code. Only the two comments and the one rule sentence in point 4.
- The writer's output for a world with no kept sections — card 070's expected texts stand.
- No file I/O, no editor call site, no project file.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R authoring` passes.
- `grep -rn 'component_set\|intent_submit' authoring/src` returns nothing.
- Paste the text-first round-trip file into Notes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A scene file becomes a world and back into the same bytes, a broken file creates nothing, and a
section the program does not understand survives the trip.

## Notes

**Verified on Linux** (Fedora 44, clang 22): `cmake -P check.cmake` exits 0 —
standalone `authoring`, includes, tests (`authoring 2`, 47 in all), descriptions off,
analyser (129 files). `ctest -R authoring`: 2 of 2 pass. `grep -rn
'component_set\|intent_submit' authoring/src` returns nothing. `tools/hot.sh`: no
`OVER` (`voe3d/CLAUDE.md` 371/400). Windows not checked.

**The text-first round-trip file** (`[2.voe_game_health]` is the kept section of a
type nothing registered):

```
[1]
name = "Camera"
[1.voe_scene_camera]
eye = [0, 2, 5]
yaw = 0
pitch = -0.25
fov_y = 1.25
near_plane = 0.1
far_plane = 100
[1.voe_scene_transform]
position = [0, 2, 5]
rotation = [0, 0, 0, 1]
scale = [1, 1, 1]

[2]
name = "Cube \"big\""
[2.test_link]
target = 3
[2.voe_game_health]
points = 20
armour = "light"
[2.voe_scene_transform]
position = [1, 0.5, -3]
rotation = [0, 0.5, 0, 0.75]
scale = [2, 2, 2]

[3]
name = "Floor"
[3.test_link]
target = 0
```

**Two `DEVIATION:` markers:**
- `src/scene_read.c` header: a scratch row is pushed at the size its description
  implies (last field's end rounded up to 8), not the registered size. ecs hands a
  type's size back only through `_replace`, which not every type has, and ecs may
  not be edited here. Where `_replace` knows the size, an assert checks the two agree.
- `src/scene_write.c`, `put_kept`: a kept section is refused if a type has been
  registered under its key name since. Writing it would give a section the reader
  refuses: a duplicate, or a runtime-only type's section. The card does not list this.

**What was done beyond the literal list, all inside `authoring`:**
- `DEPENDS` gains `assets`, which the `voe.cmake` row already allows.
- The writer's merge sort moved to `src/authored.{h,c}`. The reader uses it plus a
  binary search to find `[N]` and entity references without a walk per lookup.
- The writer's `kept` parameter sits second, after `world`.
- Extra tests: kept sections placed, dropped and refused (`tests/scene_write.c`), a
  world that runs out of room, empty text, `[02]`, `[1.voe_scene_identity]`,
  integers out of range, a negative unsigned, trailing text after an array.

**Readings taken without the planning root.** The card's "ADR-0149's table" was
read as the spellings `scene_write.h` documents, accepted with blanks tolerated
around brackets and commas. Integers are `-?digits`. Floats are
`-?d+(.d+)?(e[+-]?d+)?` and must be finite. `[N]` must have no leading zero, so
two sections cannot name one id. An `id =` key in `[N]` is an unknown key: a warning.

**Not tested: a world that already holds an authored entity asserts.**
`voe::testing` cannot catch an assert, so this case has no test. The test file's
header says so too.

**Findings for the tech lead:**
- ecs has no public accessor for a type's registered row size; hence the first
  deviation.
- A CHAR value's quotes are stripped by the sectioned reader, which does not record
  whether a value was quoted, so `name = Cube` loads like `name = "Cube"`. A kept
  section keeps its lines raw, so it round-trips either way.
- `voe_authoring_scene_write` has `const char **out_text`, from card 070, which
  rule 6 forbids. It was left alone because this card did not ask for it.
- Cards 077 and 078 (a field of any shape) appeared in `todo/` during this card.
  078 will touch `element_of`/`floats_of` in `scene_read.c` as well as the writer.
