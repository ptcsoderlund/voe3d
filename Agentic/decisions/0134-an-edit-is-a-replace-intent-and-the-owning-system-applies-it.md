# 0134. An edit is a replace intent, and the owning system applies it

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked for the first editor card on 2026-09-11 — a window, a list of what is
in the scene, and a panel to edit things in. It could not be written: an inspector changes
a field it knows only by a description — a name, a kind, an offset and a size (ADR-0127) —
and **nothing decided how that change reaches the component.**

Rule 3 of `voe3d/CLAUDE.md`: a component is written only by its own system. Rule 4: to
change another module's data, submit an intent. ADR-0065's loop names *"game, dev or
editor code"* as what submits in the `submit` phase. ADR-0132 lets the inspector find the
description for any registered type and name none of them. ADR-0121: the editor is a leaf,
and no folder gains a symbol for its sake without an ADR — this is that ADR.

What is on disk, checked the same day. `voe_ecs_component_set` is public, and every call
outside `ecs`'s tests is inside the owning module, so rule 3 is kept by convention and
nothing enforces it. Transform and light each have one intent carrying the entity and the
whole component. The camera's two intents carry part of it. Mesh and panel have direct
setters and no intent. **The light's direction becomes unit length in its add and its
typed submit, and not in its drain**; the transform's drain applies the row as given.

## Options considered

### Option A — the editor writes the row directly
Copy the row, change the bytes, call `voe_ecs_component_set`; rule 3 gains a written
exception for the editor. The cheapest. It skips every check a component makes on its own
values, silently: a light direction dragged in the inspector stays non-unit and the shading
is wrong with nothing asserting.

### Option B — a described component names a replace intent
Describing a component also names its intent carrying the entity and a whole row. The
editor copies the row, changes the field, submits; the owning system applies it with its
own checks. Rules 3 and 4 are unchanged.

### Option C — the description carries a validate function
Direct write, then a checker the component supplies. The engine expects function pointers
in one place, `render`'s Vulkan table; this makes one per component.

## Decision

**Option B**, the principal's choice on the tech lead's recommendation. The deciding
factor: **the editor is the first writer that does not know what it is writing, and the
owning system is the only place that knows what a valid value is.** B is the only option
that does not route around it.

1. **A described component names its replace intent.** The intent's value holds the entity
   at offset zero and one whole row of the component at an offset the declaring folder
   states with `offsetof` — the compiler's answer, as ADR-0122 requires of every offset.
2. **`ecs` stores the intent and the row offset beside the type's description and never
   reads either**, as it already does the description. It is its own call, made by the
   declaring folder next to its registration, **not a further parameter to registration**:
   a described component without a replace intent is a legitimate state, not a mistake.
3. **Such a component is shown and not editable.** The same soft edge ADR-0132 gives an
   undescribed one: visible in the tool, never a build failure.
4. **The inspector edits by reading, copying and submitting.** Read the row — reading is
   anybody's — copy it, change the field's bytes at the described offset, submit the
   entity and the copy. **The editor never calls `voe_ecs_component_set`.**
5. **A system's checks belong where its intent is applied.** A check run only in a typed
   submit function is bypassed by a generic submitter, so a system that checks values must
   also check them in its drain.
6. **Whole row, last writer wins** — the rule `scene/transform_system.h` already states, for
   the same reason.
7. **An edit lands when the owning system next runs**, so the editor's loop runs the owning
   systems every frame whether or not anything is playing.

## Blast radius

**Moderate.** Point 2 makes *describing* a component include naming how it is replaced, and
every editor feature that writes — the inspector, and later undo and paste — is built on
submitting that intent. Going to A later is easy and would silently strip every check;
coming back from A is a pass over every writer. Reversibility: **moderate.**

## Consequences

- **Rules 3 and 4 hold with no exception**, and the editor is one more submitter, exactly
  as ADR-0065 already drew it.
- **Transform and light cost one call each** when described: their intents already have
  this shape. **The light must also settle its direction in its drain** (point 5).
- **The camera needs a new whole-row intent** before it is editable; its placement leaves
  the lens alone and its motion is a delta. **Mesh and panel need their first intent.**
- **An edit is one frame late.** Invisible at a person's speed, and it is the price of the
  owning system seeing the value first.
- **A check that asserts is now wrong for authored values.** The light asserts on a
  zero-length direction, which is right for a program's bug and wrong for a person dragging
  a number through zero. What a drain does with a value a person made — refuse it, correct
  it, or stop — is opened below and not answered here.
- **The transform's rotation is a quaternion nothing normalises on write.** Four raw numbers
  edited one at a time will leave it non-unit. How a rotation is presented belongs to how
  the inspector edits values, which is its own question.
- **Card 050 is unaffected.** Its registration signature stands; this adds beside it.

## Rejected options and why

**A — direct write.** It turns every check a component makes into one the editor skips, and
the failure is silent and shows up in a picture rather than a message. It also writes an
exception into the one rule that makes ownership mean something, for the first caller that
most needs holding to it.

**C — a validate function.** It moves the check to where it can be forgotten: the editor
must remember to call it, and every future writer too. And it makes function pointers an
ordinary part of a component where today they are one named exception in `render`.

## Questions this opens

- **D-242** — what an owning system does with a replace carrying a value that breaks its
  invariant: keep the old row, correct the value, or assert.
