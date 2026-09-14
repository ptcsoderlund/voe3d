# 070 — `authoring` exists, and writes a scene

claimed-by: claude-opus-5 (session eeef7b29)
blocked-by: 069
decision: *The scene reader and writer live in `authoring`* (ADR-0151) points 1–3, 5, 6; the text it writes is *A scene file is entity sections by number…* (ADR-0149) — sections 1–2, the value table, points 5–6; what it writes is ADR-0150 point 1. The reader is not this card: how it may write tables it does not own is still open.

## Goal

A new folder, `authoring`, turns a world into ADR-0149's canonical scene text in an arena. It
reads the world and never writes it. Nothing calls it yet but its tests.

## Scope

**1. The folder** (ADR-0151).
- `authoring/CMakeLists.txt`, the four lines: `voe_module(authoring DEPENDS scene ecs assets math base)`
  — trim to what the code includes, within that row.
- `cmake/voe.cmake`: an `authoring` row `scene ecs assets math base`, with a comment in the
  voice of the others saying it is authoring-time code a game's build does not link, and why.
  The `editor` row gains `authoring`; if card 067 already gave it `3d`, keep that. Update the
  editor row's comment.
- Root `CMakeLists.txt`: `add_subdirectory(authoring)`, placed after `assets` and before
  `render` or wherever the file's own ordering puts a folder with these dependencies.
- `voe3d/CLAUDE.md`: the tree and the dependency table gain `authoring`; `app`'s row says
  *all* and needs no edit unless the file says otherwise.
- `authoring/authoring.md`: what the folder is (ADR-0151 point 5 — needs descriptions, not
  linked by a shipped game, why a game that turns descriptions on may), and its files.

**2. `authoring/include/authoring/scene_write.h`.**

```c
[[nodiscard]] bool voe_authoring_scene_write(const voe_ecs_world *world,
					     voe_base_arena *arena,
					     const char **out_text, size_t *out_size);
```

Match the arena and span idioms `assets/include/assets/sectioned.h` uses if they differ. The
text is NUL-terminated for convenience and `out_size` excludes the NUL.

**3. What it writes** — ADR-0149, exactly:
- **Entities that have a `voe_scene_identity`**, ascending by authored id. Each is
  `[N]` followed by the identity's fields other than `id` — today `name = "..."`.
- Beneath each, every component type **with a description** that the entity has, except the
  identity, ascending by the byte order of its key name, as `[N.<key>]`, then every field in
  declaration order as `key = value`. **Runtime-only types are skipped** (`voe_ecs_component_runtime_only`).
- A blank line before every `[N]` except the first; none elsewhere; `\n`; a final newline. An
  empty world is the empty text.
- **Values**, by the field's kind and count:
  - integers decimal; `BOOL` `true`/`false` (non-zero is true);
  - `FLOAT32`/`FLOAT64`: **the shortest decimal that reads back to the same bits** — try
    increasing precision with `%.*g` until `strtof`/`strtod` returns the identical value;
    `-0.0` is written `-0`;
  - `FLOAT2/3/4`, `QUAT`, `FLOAT4X4`: `[a, b, c]` in memory order;
  - a field with `count > 1` of a non-`CHAR` kind: `[x, y]`, nesting once for vector kinds:
    `[[0, 0, 0], [1, 1, 1]]`;
  - `CHAR` arrays: the bytes up to the first NUL in `"..."`, `"` and `\` escaped;
  - `ENTITY`: the target's authored id, or `0` for a dead entity; a live target **with no
    identity** writes `0` **and reports a warning** naming both entities.
- **It refuses — returns false, reports an error, `out_*` untouched —** on: an `ENUM` field
  (no value names exist yet — say so in the message), a `NaN` or infinite float, a `CHAR` value
  containing a byte below `0x20`, two entities with the same authored id.

**4. `authoring/tests/scene_write.c`**, no graphics card.
- A world with transform and identity registered and three authored entities plus one without
  identity: the output equals an expected string literal **byte for byte**, including order,
  blank lines and the final newline.
- A test-only described component with one field of every kind the writer supports, including
  a fixed array of `FLOAT3` and a `CHAR` array holding `"` and `\`: exact expected text.
- Floats: `0.1f` writes `0.1`, `1.0f` writes `1`, `-0.0f` writes `-0`, `16777217.0` as
  `FLOAT64` round-trips; write each, `strtod` it back, compare bits.
- A runtime-only component on an authored entity does not appear.
- Entities created in id order 3, 1, 2 come out 1, 2, 3.
- An `ENTITY` field naming an entity without identity writes `0`.
- Each refusal above returns false.
- Writing the same world twice gives identical bytes.

## What must not change

- No world is written to; the function takes `const voe_ecs_world *`.
- `scene`, `ecs`, `assets`, `base` — no edit. A gap in one is a report.
- No reader, no file I/O, no project file, no editor call site.
- No unknown-section preservation — that needs the reader.

## Verify

- Linux: `cmake -P check.cmake` green, including its folder-list and dependency-edge steps
  seeing `authoring`; `ctest -R authoring` passes.
- Paste the three-entity test's expected text into Notes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER` — `voe3d/CLAUDE.md` is near
  its ceiling, so keep the additions to the lines the table and tree need.

## Done looks like

A world goes in and ADR-0149's text comes out, the same bytes every time, proven against
literal expected files.

## Notes

**Verified on Linux** (Fedora 44, clang 22): `cmake -P check.cmake` exits 0 —
`authoring` in the folder list, standalone, includes, tests (`authoring 1`) and the
analyser (126 files). `ctest -R authoring` passes. `tools/hot.sh`: no `OVER`
(`voe3d/CLAUDE.md` 369/400). Windows not checked.

**The three-entity test's expected text:**

```
[1]
name = "Cube"
[1.voe_scene_transform]
position = [1, 2.5, -3]
rotation = [0, 0, 0, 1]
scale = [1, 1, 1]

[3]
name = "Floor \"big\""
[3.voe_scene_transform]
position = [0, -0.5, 0]
rotation = [0, 0, 0, 1]
scale = [10, 0.1, 10]

[7]
name = "Sun"
```

**Two `DEVIATION:` markers, both in `authoring/src/scene_write.c`:**
- `%.*g` alone spells 10 as `1e+01`. The writer takes the fewest digits `%.*g`
  finds, then writes them with or without the exponent, whichever is shorter
  (plain on a tie). So 10 is `10` and 1e-05 stays `1e-05`. Tested.
- A described type whose description this build compiled out is refused too,
  because `ecs/component.h` says a writer refuses rather than saving nothing.

**Findings for the tech lead:**
- `describe.h` lets only ENUM, CHAR and ENTITY repeat, so the "fixed array of
  `FLOAT3`" this card tests cannot be declared through `VOE_BASE_DESCRIBE_STRUCT`.
  The every-kind test writes its description table by hand, which the world
  accepts. The writer's nesting is proven, but no real component can have one yet.
- Section names use the key's full name, `[1.voe_scene_transform]`, because
  `voe_ecs_key.name` is what the card says to sort by.
- `authoring/CMakeLists.txt` DEPENDS `scene ecs base`, trimmed from the row. The
  `voe.cmake` row allows `scene ecs assets math base`; editor's row gains
  `authoring`, but editor's own CMakeLists is untouched because nothing calls it yet.
- Refusals leave the arena's pushes for the caller to rewind, as `sectioned.h` does.
