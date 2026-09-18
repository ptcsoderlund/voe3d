# 0137. The identity component lives in `scene`

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-244**: *which folder holds the identity component.* ADR-0125 fixed its shape — a
64-bit id unique within its file and a 64-byte name — made it optional, made it ship, and
made its presence mean *authored*. It wrote the type as `voe_identity`, which has no folder
in it and so breaks rule 7's prefix; the folder decides the spelling. The first editor card
lists exactly the entities carrying one, so it cannot be written until this is.

What is on disk. `ecs/ecs.md`: *"Not any particular component: nothing here knows what a
transform or a mesh is, and the folder that owns a component's meaning is the folder that
registers it."* `ecs` holds no component and no system. `scene/scene.md`: *"the components
a person would author, and the systems that own them"* — transform, camera and light, each
a component header and a system header. Every reader named so far — the editor, a scene
loader and saver, a shipped game's log line — sits above `scene` in the module map.
ADR-0134: a component a person edits needs a system with a whole-row replace intent.

## Options considered

### Option A — `scene`
Beside transform, camera and light, spelled `voe_scene_identity`, with its own system.

### Option B — `ecs`
The lowest folder; everything entity-aware sees it, and `ecs`'s own messages could name an
entity. It gives `ecs` its first component, its first system and a meaning.

### Option C — a new folder between `ecs` and `scene`
A module-map change and a dependency row for one struct.

## Decision

**Option A**, the principal's choice on the tech lead's recommendation. The deciding
factor: **`scene` already defines itself as the home of what a person authors, and this
component's entire meaning is that a person authored the entity.**

1. **`scene/include/scene/identity_component.h` and `identity_system.h`**, the same pair
   transform, camera and light each have. The type is `voe_scene_identity`; ADR-0125's
   `voe_identity` was the shape, not the spelling.
2. **It is declared through `base/describe.h`** (ADR-0127): the id as a 64-bit unsigned
   integer, the name as a 64-element `CHAR` array.
3. **Its system is the only writer.** It registers the table, the description and the
   whole-row replace intent ADR-0134 requires, offers a direct creation call as the others
   do, and drains that intent. A rename is that intent.
4. **What it does not own.** Choosing an id, keeping ids unique within a file, and making
   up a name such as `Cube_2` belong to whoever creates authored entities — the editor and,
   later, the loader (ADR-0125 points 4 to 6). There is no lookup by id or by name: a caller
   walks the table until a second caller wants one (rule 10).
5. **`ecs` and the module map are untouched.**

## Blast radius

**Cheap.** Moving it later is a rename and a folder move, and no saved file records which
folder a component came from. What would cost more is B's direction: once `ecs` owns one
component with a meaning, the argument against a second is gone.

## Consequences

- **A `scene` card comes before the editor card**, beside the `app`, `dev` and number-box
  cards the last two decisions put there.
- **`ecs`'s own messages cannot name an entity.** A stale-id assert still prints an index
  and a generation. Accepted: the caller that holds the id is above `scene` and can name it.
- **A program using `ecs` without `scene` has no names.** Nothing does that today.
- **Whether the glTF importer gives what it creates an identity** is not decided here; it
  becomes possible because `3d` already depends on `scene`.

## Rejected options and why

**B — `ecs`.** It contradicts the one thing `ecs` is for, and ADR-0125 already rejected
putting names and authored-ness into `ecs` on that ground, for identity as a field of the
entity. A component inside `ecs` is the same objection one step removed.

**C — a new folder.** A folder for one struct is structure ahead of need. It becomes worth
revisiting if authoring grows a family of its own — named templates (D-175) or a file's own
identity (D-237) — and that is a later decision, not this one.

## Questions this opens

None.
