# 0125. Identity is one component, it ships, and its presence means authored

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-235 and D-098.** A scene needs stable ids so one entity can reference another
and survive being saved and reloaded. ADR-0124 settled that a reference is a runtime entity
id; a runtime id is an index and a generation describing *this* run, so the number written
to a file means nothing the next time the program starts.

The principal's requirement, 2026-09-11: *"If entity id is a reference it should be stored
so it has the same id every time editor and game starts."* And his objection to the tech
lead's first answer, which is what shaped this one: *"If all entities have an identity
component, then the identity component IS the entity. Some entities don't need to be
identified, only carry data."* He is right — a component present on everything is a field
of the entity wearing a disguise.

His second proposal was to compile the name out of the shipped build:
*"maybe we do name only for #VOE_EDITOR? In runtime it doesn't exist."*

Fixed by earlier decisions: ADR-0123's vocabulary — primitives, fixed-size arrays, entity
references — so a name is a `char` array and an id is an integer. ADR-0122: the editor
build and the shipped build must not disagree about a struct's shape. ADR-0073: the
authored format is text, chosen to be read and diffed.

## Options considered

### Option A — identity is a field of the entity itself
Every entity has an id and a name, in the world's own slot table. Simple lookup, no
component to add or forget.

### Option B — two components: an id that ships, and an editor-only name
`voe_identity { uint64_t id; }` in both builds; `voe_editor_name { char name[64]; }`
registered only by the editor and stripped at cook. Nothing editor-shaped reaches the
product.

### Option C — one component carrying both, in both builds
`voe_identity { uint64_t id; char name[64]; }`, optional, present on authored entities only.

## Decision

**Option C**, and the tech lead's earlier recommendation of Option B is withdrawn.

**1. One component, carrying the id and the name, in both builds.**

```c
typedef struct { uint64_t id; char name[64]; } voe_identity;
```

**The deciding factor is where a name is actually worth something, and it is not the
editor.** In the editor you can click the thing; you already know what it is. In a shipped
build a log line or a crash report reading `entity 4821` is useless and one reading
`Spawner_Courtyard_North` says where to look. Option B removes the name at precisely the
moment it earns its keep.

The cost it avoids is negligible and recoverable. Only authored entities carry identity —
runtime-spawned ones never do — so it is tens of kilobytes in a product shipping megabytes
of textures, and if it is ever judged dead weight the cook strips that one component with
nothing else changing. Keeping it costs a rounding error and preserves an option; removing
it now closes one and buys almost nothing.

**64 bytes, not 32.** `Enemy_Spawner_Courtyard_North` does not fit in 32. Raising the cap
later is compatible, because the text file holds the string and only the struct carries the
limit.

**2. It is optional, and its presence means the entity was authored.** This is the
principal's objection turned into the design. An entity created at runtime — a bullet, a
particle, debris — has no identity component, has no stable id, and dies with its
generation. **A system may reference it and simply loses the reference when it disappears**,
which is what the generation already delivers.

**3. Presence is the save rule.** The saver writes entities that have an identity and skips
the rest. Saving while the game runs writes the scene and not the two hundred bullets in
flight, with no special case anywhere. This property is only available because identity is
*not* universal.

**4. Every entity the editor creates gets one, and the editor fills the name in.** The
editor cannot know in advance which entities something will end up referencing, and a blank
name is a bad first impression, so creation assigns both — `Cube`, `Cube_2`, in the manner
of Unity and Godot — and renaming is the ordinary act.

**5. Ids are unique within their file, not globally, and they are numbers rather than
GUIDs.** A reference that crosses files is the file's own identity plus the id within it.
The authored format is text meant to be read and diffed in git, and `target = 7` survives a
review and a merge conflict in a way a 36-character GUID does not. The merge safety a
per-entity GUID buys only matters when two people edit **the same scene file**, which is
the named trigger to revisit this.

**6. Loading maps authored ids to runtime entities.** Entities are created in file order,
a map from authored id to runtime entity is kept for the duration of the load, every
reference field is patched through it, and the map is discarded. **This is required
regardless**, because instantiating one template fifty times means fifty copies of the same
authored ids — which is why making the runtime id equal the authored id was not chosen.

## Blast radius

**Load-bearing.** Point 5 is in every saved file: moving to GUIDs later means converting
every scene. Points 1 to 4 are cheap — the component is one struct, and whether it ships
is a line in the cook.

## Consequences

- **A shipped game can name its own entities in a log.** That is the payoff and it was
  nearly designed out.
- **Two people editing the same scene file can collide on an id.** Accepted knowingly, with
  the trigger named. It is an editor problem with an editor answer — renumber on merge —
  and not a reason to make every reference unreadable today.
- **The outliner lists exactly the entities with identity**, which is the same set the
  saver writes. One concept answers what is authored, what is named, and what is saved.
- **An entity can be referenced only if it is authored.** A runtime entity cannot be the
  target of a saved reference, because it has nothing stable to write. This is correct and
  it will surprise somebody: a designer cannot point a saved field at a spawned bullet.
- **Identity is not a transform.** An entity with an identity and no transform is ordinary
  — a settings object, a match state, a spawn table. This engine already has Godot's shape
  rather than Unity's forced transform, and this ADR does not change it.

## Rejected options and why

**A — a field of the entity.** Rejected on the principal's own grounds: it makes every
entity pay for a name and an id including the bullets, and it puts game-facing data in
`ecs`, which deliberately knows nothing about what anything means. It also loses point 3 —
if every entity has an id there is nothing to distinguish authored from spawned, and the
saver needs a separate mechanism.

**B — the name compiled out of the shipped build.** Rejected on the argument in point 1.
Worth recording that it was the tech lead's own recommendation one exchange earlier, made
while solving *how do I honour the principal's request safely* rather than asking whether
the request was the best shape. The safe mechanism it produced is kept — see ADR-0126 —
and simply is not applied to names.

## Questions this opens

- **D-237** — what the *file's* own identity is, given ADR-0125 point 5 makes a cross-file
  reference depend on one. A path is readable and breaks on rename; a GUID in a header
  survives a rename and is unreadable. Nothing needs it until a second authored file exists.
