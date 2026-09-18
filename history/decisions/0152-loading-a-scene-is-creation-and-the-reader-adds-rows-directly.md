# 0152. Loading a scene is creation, and the reader adds rows directly

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** —. Amends `voe3d/CLAUDE.md` rule 3 by one stated exception
- **Superseded by:** —

## Context

**Closes D-261**, opened by ADR-0151: *how a loaded scene's rows reach tables `authoring` does
not own.*

`voe3d/CLAUDE.md` rule 3: a component is written only by its own system. Rule 4: anyone else
submits an intent. The scene reader in `authoring` (ADR-0151) must create entities and give them
every described component — identity, transform, camera, light, and types it has never heard
of by name, a game's own included (ADR-0132: a new component needs no edit anywhere else).

**Creation is already the exception in practice.** `scene/include/scene/transform_system.h`:
*"creation is a direct call and not an intent … giving an entity its first transform is what
makes it exist, and it happens where the entity is being built rather than in the middle of a
frame. The importer uses this."* The glTF importer creates transforms from a file that way, and
nothing validates them until an intent touches them. What the reader needs is the same act done
generically, which `voe_ecs_component_add(world, type, entity, bytes)` already is.

## Options considered

### Option A — loading is creation
The reader creates entities and adds each component's bytes directly. Stated as an exception:
only to entities it created in that load, never to an existing row. Any described type loads;
a value a drain would correct loads as written, as the importer's do.

### Option B — every row through its owner's replace intent
Add zeroes, submit every loaded row as a replace intent, and let each drain settle it.
Everything is validated; a scene is only right after the systems run once; a type without a
replace intent — camera and light today — cannot be loaded at all.

### Option C — each folder supplies its own loader
`authoring` calls every owning folder by name. A game's components cannot load, ADR-0132's
promise breaks, and it needs a function pointer or a switch over every type.

## Decision

**Option A**, the tech lead's recommendation, the principal's call.

> *"yes option 1"*

1. **A load creates.** The scene reader creates one entity per `[N]` and adds each of its
   described components with `voe_ecs_component_add`, the bytes built from the file through the
   type's description.
2. **The exception to rule 3 is exactly this and no wider**: `authoring`'s reader may add rows
   **to entities it created in the same call**. It never calls `voe_ecs_component_set`, never
   removes a row, and never touches an entity it did not create. `voe_ecs_component_add` being
   public does not widen this for anyone else; rule 3 still binds every other caller, and the
   typed creation calls folders already expose remain how code that knows its types creates them.
3. **References are resolved before anything is added.** Every entity is created first, the map
   from authored id to runtime entity is built (ADR-0125 point 6), and each row's `ENTITY` fields
   are patched in the scratch bytes before its add — so no row is ever written twice.
4. **A file is validated whole before the first entity is created.** Every ADR-0149 refusal is
   found in a first pass over the parser's text into scratch memory; only a file that passes is
   created. A scene is never half loaded because the file was wrong. A world that runs out of
   room during creation — a capacity the program chose — returns false and the caller discards
   that world; the reader says so in its header rather than pretending to undo.
5. **A reference to an id the file does not contain loads as no entity, with a warning.** This is
   the one case ADR-0149 point 10 did not name: well-formed text naming an entity that was deleted
   by hand. Refusing the file would make a scene unopenable over one stale number; a warning and
   an empty reference is the local failure the format was chosen for.
6. **Values a drain would correct are loaded as written.** A hand-edited rotation that is not
   unit length stays so until an intent touches it, exactly as the importer's creations do today.
   Nothing in `authoring` knows what any field means.
7. **The world handed to the reader holds no authored entity.** Loading beside an existing scene
   would make two entities claim one authored id; that is the caller's bug and asserts.

## Blast radius

**Moderate, and it is a rule's wording.** Rule 3 now carries a named exception, in
`voe3d/CLAUDE.md` and in `ecs/include/ecs/component.h`'s *read by anyone and written by one*
paragraph. Narrowing it later — routing loads through intents — is a change to `authoring`
alone, plus giving every loadable type a replace intent. The reader's code is cheap.
Reversibility: **moderate.**

## Consequences

- **A game's own components load in the editor the moment they are registered and described**,
  which D-254 will need whichever way it is answered.
- **A bad value typed into a file loads.** If that is ever found hurting — somebody complains, or
  a drain reports a settling on the first edit of a freshly opened scene often enough to notice —
  the answer is to submit loaded rows through replace intents where a type has one, as a second
  pass, and it is a card in `authoring`.
- **Two exceptions to rule 3 now exist and they are the same one**: a folder's typed creation
  call, and the reader's generic add. Both create; neither edits.
- **The reader card can be written**, and it extends card 070's writer to put back the sections
  it kept but could not read (ADR-0149 point 9).

## Rejected options and why

**B — through replace intents.** Validation bought at the price of types that cannot be loaded at
all, and a world that is wrong until systems have run.

**C — each folder's loader.** It cannot load a type `authoring` was not written against, which is
every game's own component.

## Questions this opens

None.
