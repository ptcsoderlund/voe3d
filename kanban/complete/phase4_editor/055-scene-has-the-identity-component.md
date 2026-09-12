# 055 — `scene` has the identity component: an id and a name on authored entities

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: 053, 060
status: review
decision: *Identity is one component, it ships, and its presence means authored* (ADR-0125) for its shape and meaning; *The identity component lives in `scene`* (ADR-0137) for its folder, spelling and what it does not own; *An edit is a replace intent* (ADR-0134) and *A drain corrects a bad value or keeps the last valid one, and says so* (ADR-0138) for how it is written; *A field can be marked read-only, and an authored id is never replaced* (ADR-0139) for its `id`.

## Goal

`voe_scene_identity` — a 64-bit id and a 64-byte name — registered with its description,
added directly, replaced through its intent, and settled in its drain. Its id is read-only
and cannot be changed by a replace.

## Scope

**1. `scene/include/scene/identity_component.h`, `scene/src/identity_component.c`**, shaped
as the transform's pair is, with the field list in the form card 060 gives it:

```c
#define VOE_SCENE_IDENTITY_NAME 64

#define VOE_SCENE_IDENTITY_FIELDS(F, F_READ_ONLY)      \
	F_READ_ONLY(uint64_t, id, UINT64)                \
	F(char[VOE_SCENE_IDENTITY_NAME], name, CHAR)

VOE_BASE_DESCRIBE_STRUCT(voe_scene_identity, VOE_SCENE_IDENTITY_FIELDS)

extern const struct voe_ecs_key voe_scene_identity_key;
```

plus `_get`, `_count`, `_rows` and `_entities`, as `transform_component.h` has them. The
header says: the component is optional and its presence means the entity was authored; the
id is unique within its file, set when the entity is created, and never changed after; there
is no lookup by id or by name.

**2. `scene/include/scene/identity_system.h`, `scene/src/identity_system.c`.**

- `voe_scene_identity_register(world, capacity)`: the table with its description (NULL when
  descriptions are off, exactly as `scene/src/transform_system.c` does it); the intent
  `voe_scene_identity_intent { voe_ecs_entity entity; voe_scene_identity identity; }`; and
  `voe_ecs_component_replace_set(..., offsetof(voe_scene_identity_intent, identity))`.
- `voe_scene_identity_add` and `voe_scene_identity_submit`, the typed calls. **Both assert
  that the name has a terminating zero within its 64 bytes** — a program's bad value.
- `voe_scene_identity_system_run`, the drain. **Its two rules, written in the component's
  header**, both corrections reported as `warning:`:
  - a name with no terminating zero has its last byte set to zero;
  - **an id different from the entity's current id is put back to the current id.**
  An intent for an entity with no identity is dropped, as the transform's drain drops one.
- **Reporting.** When a drain settles an intent after a drain that settled none, one line on
  stderr, e.g. `warning: voe_scene_identity: entity 4v1: name was not terminated, cut to 63
  bytes` or `warning: voe_scene_identity: entity Cube_2: id cannot be replaced, kept 2`.
  When a drain settles none after such a run: `warning: voe_scene_identity: <n> intents
  settled in that run`. The run flag and count are file-scope statics in
  `identity_system.c`; its header says they are per process and not per world, and that
  this is enough for a report.

**3. Tests — `scene/tests/identity.c`.**

- Add, get and count; an intent lands only when the system runs.
- `voe_ecs_component_replace` for the identity's type is `set`, with the offset equal to
  `offsetof(voe_scene_identity_intent, identity)`.
- An unterminated name submitted raw through `voe_ecs_intent_submit` — not the typed call —
  arrives terminated, and the rest of the row arrives as submitted.
- A replace carrying a different id arrives with the entity's current id and its name as
  submitted.
- With descriptions on: two fields — `id` of kind `UINT64`, count 1, read-only; `name` of
  kind `CHAR`, count 64, not read-only.

**4. `scene/scene.md`** — the four new files.

## What must not change

- Transform, camera and light, and their systems.
- **No id allocation, no uniqueness check across entities, no default name, no lookup by id
  or name.** Ids are unique within a file, not within a world (ADR-0125 point 6); the drain
  compares an id only with the same entity's current one.
- `ecs` is not edited; `scene`'s `DEPENDS` is not edited.

## Verify

- Linux: `cmake -P check.cmake` green.
- `ctest -R scene` passes with descriptions off (`--preset debug`) and on (a scratch tree
  with `CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1`, as cards 048 and 050 did).
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

An entity with a name a log line can print, a rename that goes through the door the editor
will use, and an id nothing but its creation can set.

## Notes

**Verified on Linux**, clang 22, 2026-09-12. `cmake -P check.cmake` exits zero — standalone
`scene` included, 42 tests, analyser over 116 files. `ctest -R scene` passes with descriptions
off (`--preset debug`) and on (scratch tree, `CMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1`); the
whole 42 pass in that tree too. `tools/hot.sh` at the root: no `OVER`, `scene.md` 59/120.
**Windows unchecked** — nothing here is platform-aware, but that is the note a later bug
report gets written against.

**One question asked and answered, not guessed.** The card's two example warning lines name
the entity two different ways — `entity 4v1` and `entity Cube_2` — and state no rule. The
principal chose: *the name the intent itself carries when that name is terminated, otherwise
`<index>v<generation>`*. That is the only single rule that yields both examples, and it is
written in `identity_system.c` beside `label_of`. The test prints all three line shapes once
in a passing run, so the format is read rather than assumed:

```
warning: voe_scene_identity: entity 0v1: name was not terminated, cut to 63 bytes
warning: voe_scene_identity: 1 intents settled in that run
warning: voe_scene_identity: entity Cube_2_renamed: id cannot be replaced, kept 2
```

**Reading taken, narrowly:** "settles" in the reporting sentences means *corrects*, not
*applies* — the card calls both rules corrections and reports them as `warning:`, and the
other reading would log every ordinary rename. Written as "corrected" in the code.

**For the tech lead, reported and not acted on:** card 062 lists the folders whose
`fprintf(stderr, ...)` sites move onto `VOE_BASE_WARNING` — `render`, `assets`, `ui`, `3d`,
`app` and `dev`. **`scene` is not on that list, and this card puts two stderr sites in
`scene/src/identity_system.c`.** They are both in `voe_scene_identity_system_run`. Card 062
was written before these existed; left alone here because it is another card's folder and
another card's job.

**Suggestion, not done:** nothing tests the report itself — the lines are printed and looked
at, not captured. Capturing stderr would need `freopen` in a test that also reports failures
there, which is worth its own decision rather than a quiet addition here.
