# 048 — A struct describes its fields, and `scene`'s transform is the first one

claimed-by: claude-opus-5 (session 015U4t8aynAxkavRbfYdkdKX)
blocked-by: -
status: review
decision: *A struct describes its fields, and the description lives in `base`* (ADR-0127) — a description is a name, a kind, an offset, a size and a count; the kind is an enum so `base` depends on nothing new; the declaring folder supplies the C type; the struct is always generated and the table only when asked. The vocabulary of kinds is ADR-0123. Why a struct and its description are written once together is ADR-0122.

## Goal

`base/include/base/describe.h` exists. `voe_scene_transform` is declared through it, its
layout unchanged. A test proves every described offset is the compiler's own and that the
declared kind matches the declared type.

## Scope

**1. `base/include/base/describe.h` — the type and the macros.**

- An enum of field kinds covering ADR-0123's vocabulary: the sized integer and
  floating-point types, `BOOL`, `FLOAT2` / `FLOAT3` / `FLOAT4`, `QUAT`, `FLOAT4X4`, `ENUM`,
  `CHAR` (for fixed-size name arrays) and `ENTITY`. Spell them `VOE_BASE_FIELD_*`.
- A field record: name, kind, offset, size, and element count (1 for a scalar, N for a
  fixed-size array).
- A struct record: the struct's name, a pointer to its fields, and how many.
- A declaration macro taking a struct name and a field-list macro. The field-list macro
  takes one parameter `F` and invokes it once per field as **`F(type, name, KIND)`** — the
  declaring folder supplies the C type, so nothing here maps a kind onto a type and `base`
  includes neither `math` nor `ecs`.
- The macro expands to **the `typedef struct` always**, and to the static field table and
  an accessor returning the struct record **only when descriptions are compiled in**.
- **A compile-time size check per field.** `base` knows each kind's expected size in bytes;
  assert it against `sizeof` of the declared type with `static_assert`, so a field declared
  `FLOAT3` whose type is a `float2` fails the build. `ENUM`, `CHAR` and `ENTITY` are the
  three where the check is against the element size rather than the whole field.
- The switch that includes or excludes the tables: one macro, off by default in a build
  that has not asked for it, and its name and meaning documented in the header.

**2. `scene/include/scene/transform_component.h` — declare the transform through it.**

The struct today is `voe_math_float3 position; voe_math_quat rotation; voe_math_float3
scale;`. Rewrite it as the field list plus the declaration macro. **The struct's layout,
field names, field order and public API do not change** — `voe_scene_transform_matrix` and
`voe_scene_transform_key` are untouched.

**3. Tests.**

- `base/tests/describe.c` — a struct declared locally in the test, covering a scalar, a
  vector, a fixed-size `char` array and an entity-kind field. Check names, kinds, counts,
  and that every offset equals `offsetof` for that field. Needs no graphics card and no
  window system.
- `scene/tests/transform.c` — add to the existing file: the transform's description has
  three fields named and kinded as declared, and each offset equals `offsetof`.

## What must not change

- **No component other than the transform is described.** Not the camera, not the light,
  not a `ui` panel. This card proves the mechanism on one.
- **`base` gains no dependency.** It must not include a `math` or an `ecs` header. If
  something appears to require one, that is a `BLOCKED:` and not a workaround.
- **The module map is untouched** — ADR-0022's DAG stands exactly as it is.
- **No identity component, no editor, no serializer, no inspector.** Those are later cards.
- `voe_scene_transform`'s size, layout and behaviour are identical before and after, with
  descriptions compiled in or out.
- `base/base.md`'s opening sentence currently reads *"Memory, containers, strings and
  assert"*. It needs widening for this file — that is in scope and is one line.

## Verify

- Linux: `cmake -P check.cmake` green.
- Both new tests pass under `ctest`.
- Build once with descriptions off and once with them on: `ctest` green both ways, and the
  engine's behaviour identical.
- `grep -rn 'math/\|ecs/' base/include base/src` returns nothing.
- From the planning root, `sh tools/hot.sh` reports no new `OVER`.

## Done looks like

A transform component that reads as a list of its fields, a table beside it that says where
each one lives, and a test that would fail if either drifted from the other. Nothing else in
the engine behaves differently, and a build that does not ask for descriptions does not
contain them.

## Notes

**Verified on Linux only** (Fedora 44, clang 22.1.8, CMake 4.3.0, slangc 2026.13.1). Windows
is unchecked; the four platform sizes `describe.h` assumes (float, double, bool, a plain
enum) are `static_assert`ed in the header, so a Windows difference fails the build there.

- `cmake -P check.cmake` exits 0: 40 tests (base 3, scene 3), analyser clean over 107 files.
- Built twice more: `--preset debug` (off), and a scratch tree with
  `CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1` (on). `ctest` 40/40 both ways.
- `voe_scene_transform` is 40 bytes, align 4, offsets 0 / 12 / 28 — measured before the
  change, and after it with descriptions off and on.
- `voe_dev`'s `.text` is byte-identical off and on (203 364 bytes). `nm` finds no
  `_description` symbol in `libvoe_scene.a` or `voe_dev` in either build: nothing in `src/`
  calls the accessor, so even a build that asks emits no table until card 050 uses one.
- Scratch probes, not committed: `FLOAT3` on a two-float struct and `ENTITY` on 12 bytes
  both fail to compile, naming the field and the kind. Arrays of `ENTITY[3]`, `ENUM[2]` and
  `char[5]` describe with counts 3, 2, 5.
- `grep -rn 'math/\|ecs/' base/include base/src` is empty. `tools/hot.sh`: no `OVER`.

**Readings taken where the card left a choice** — none contradicts a rule, so no
`DEVIATION:` markers:

- The sized floats are `FLOAT32` / `FLOAT64` ("sized" in the card). A slip between
  `FLOAT3` and `FLOAT32` is caught by the size check.
- `size` is the whole field; the element size is `size / count`.
- A fixed-size array is spelled as its type, `F(char[13], label, CHAR)`, which is why the
  member is declared `typeof(type) field;`.
- The switch is `VOE_BASE_DESCRIPTIONS`, on when defined non-zero, and read at a
  translation unit's first include. Everything emitted is `static`, so a description's
  address differs between files — worth knowing for 050, which stores the pointer.
- Both tests `#undef`/`#define` the switch themselves, because `check.cmake` builds with it
  off and a test that followed the build would check nothing on the gating run.
- The macro takes no trailing semicolon: an extra one at file scope is `-Wextra-semi`,
  an error here.

**Beyond the two `.md` lines the card named:** `scene/scene.md`'s line for
`transform_component.h` now says the struct is a described field list, and `base/base.md`
gained lines for `describe.h` and `tests/describe.c`.

**Suggestion, not done:** the kind-versus-type refusal is proven only by the probes above.
A `check.cmake` step that compiles a deliberately wrong declaration and expects the
`static_assert`, as the guard steps do for configuration, would keep it proven.
