# 060 — A field can be marked read-only in its description

claimed-by: kanban-coder (Claude Opus 5)
blocked-by: -
status: review
decision: *A field can be marked read-only, and an authored id is never replaced* (ADR-0139) points 1 and 2 — the declaring field list marks a field read-only, `base`'s field record carries the mark, the struct is unchanged, and a tool shows a marked field without editing it.

## Goal

A field list can say that a field is read-only, and `voe_base_field_description` says so
for that field. Nothing about the struct it declares changes.

## Scope

**1. `base/include/base/describe.h`.**

- **A field list takes two parameters, `F` and `F_READ_ONLY`,** each invoked with the same
  three arguments as today: `(type, name, KIND)`. A field listed through `F_READ_ONLY` is
  read-only; one listed through `F` is not.

```c
#define THING_FIELDS(F, F_READ_ONLY)   \
	F_READ_ONLY(uint64_t, id, UINT64) \
	F(voe_math_float3, position, FLOAT3)
```

- `voe_base_field_description` gains `bool read_only`.
- `VOE_BASE_DESCRIBE_STRUCT` hands the list the same member expansion for both parameters and
  the same size check for both, so **the struct and its checks are identical whichever a
  field is listed through**. Only the table's row differs: `.read_only = true` through
  `F_READ_ONLY`.
- With descriptions off, nothing new is emitted.
- The header's example and its paragraphs are updated to the two-parameter form, with one
  paragraph saying what read-only means — a tool shows the field and does not edit it; the
  struct and the program are unaffected; the mark is the declaring folder's to write, beside
  the field.

**2. Call sites this breaks — both updated here (ADR-0113).**

- `base/tests/describe.c`, `THING_FIELDS`: the two-parameter form, with one of its fields
  listed through `F_READ_ONLY`.
- `scene/include/scene/transform_component.h`, `VOE_SCENE_TRANSFORM_FIELDS`: the
  two-parameter form. **None of its three fields is read-only.**

**3. Tests — `base/tests/describe.c`.** The read-only field's record says `read_only`, every
other field's does not; every offset still equals `offsetof`; the struct's `sizeof` is the
same as a struct written by hand with the same members.

## What must not change

- **The struct any field list declares**: its members, order, size and alignment.
  `voe_scene_transform` stays 40 bytes with offsets 0, 12 and 28, as card 048 measured.
- The kinds, their sizes, and the size checks.
- `ecs`, and every other folder but `base` and the one `scene` header named above.
- No other property of a field — no hidden, no range, no unit.

## Verify

- Linux: `cmake -P check.cmake` green.
- `ctest` passes with descriptions off (`--preset debug`) and on (a scratch tree with
  `CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1`).
- `voe_scene_transform`'s size and offsets are unchanged in both builds; say in Notes how
  that was measured.
- `grep -rn '_FIELDS(F)' base scene` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A field list where one line reads `F_READ_ONLY`, a description that says so, and a struct
byte-for-byte what it was.

## Notes

Verified on Linux (Fedora, clang 22, software rasteriser), 2026-09-12.

- `cmake -P check.cmake` exits 0 — 41 tests, analyser over 113 files, all folders
  standalone.
- `ctest` passes 41/41 in both `--preset debug` (descriptions off) and
  `--preset editor`, which is the descriptions-on preset that already existed
  (`CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1`) — so no scratch tree was needed.
- `voe_scene_transform` layout measured by compiling a throwaway program against
  `scene/include/scene/transform_component.h` twice, once with
  `-DVOE_BASE_DESCRIPTIONS=1` and once without. Both print
  `sizeof=40 align=4 position=0 rotation=12 scale=28` — card 048's numbers.
- `grep -rn '_FIELDS(F)' base scene` returns nothing.
- From the planning root, `bash tools/hot.sh` reports every hot file `OK`, no
  `OVER`.
- The read-only check is live, not vacuous: a copy of `base/tests/describe.c`
  with `owner`'s expectation flipped to `false` fails with
  `actual: 1 expected: 0`. The copy was compiled in the scratch directory and
  the repository file was never edited to prove it.

How it is built: `VOE_BASE_DESCRIBE_STRUCT` hands the field list the *same* macro
for both parameters when declaring members and when checking sizes, so the two
routes cannot diverge; only the table passes two different macros, and both are
one line onto a shared `VOE_BASE_DESCRIBE_ROW_IMPL_` that differs in the
`.read_only` argument alone.

`bool read_only` was added at the end of `voe_base_field_description`, which
leaves the existing positional initialisers in tests valid; they were updated to
say `false`/`true` explicitly anyway.

`base/base.md`'s `describe.h` line now says the header explains what the
read-only mark does and does not mean.

No `DEVIATION:` and no `BLOCKED:` markers.

Suggestion, not done here (outside the card): `scene/tests/transform.c`'s
`check_field` does not look at `read_only`. Card 055 gives `scene` its first
read-only field and is the natural place for that assertion.
