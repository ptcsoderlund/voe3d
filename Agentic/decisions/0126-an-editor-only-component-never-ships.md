# 0126. An editor-only component never ships

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal, 2026-09-11: *"Maybe we have `#if VOE_EDITOR` components which describes how
user can interact in the editor windows with add and remove on components and entities"*,
and later, concretely: *"maybe we do name only for `#VOE_EDITOR`? … so `char
editor_name[32];` would be the new prop."*

The instinct is right and it is the half of O3DE worth copying: the person who writes a
component writes how it is edited, next to it. What needs deciding is the mechanism,
because the obvious spelling breaks a rule taken one decision earlier.

ADR-0122 requires the editor build and the shipped build to agree about a struct's shape,
so that what was tuned is what ships. A field inside `#if VOE_EDITOR` breaks exactly that:
the struct changes size, and with it the description ADR-0122 generates, so the editor and
the game disagree about what a component *is*. The failures are silent and they appear only
in the finished product.

ADR-0052 already provides the place a strip could happen: three builds — engine, editor,
cooked game — and cooking means building again.

## Options considered

### Option A — `#if VOE_EDITOR` around fields inside a component
Exactly as proposed. Smallest change, keeps everything in one struct.

### Option B — editor-only whole components
The editor registers component types the game does not. No shared struct changes shape.
The cook strips them from the scene it produces.

### Option C — a parallel editor-side store keyed by entity
Editor data lives outside the ECS entirely, in its own table.

## Decision

**Option B.** The switch goes around a **whole component**, never around a field inside one.

1. **A component type is either registered by both builds or by the editor alone.** No
   struct differs between them. The editor's world simply holds more component types than
   the game's.
2. **The cook strips editor-only components** from the scene it produces, so they do not
   reach the product even as bytes.
3. **A loader that meets a component type it does not know skips it and continues.** This
   makes the strip a belt-and-braces measure rather than a correctness requirement, and it
   is what lets a scene saved by a newer editor open in an older build minus the parts it
   cannot understand.
4. **This is the mechanism for everything the editor knows and the game must not**:
   designer notes, gizmo colour, grouping in the outliner, a locked flag, a comment saying
   why a number is what it is.
5. **A name is not one of them** — ADR-0125 keeps the name in both builds, on its own
   merits. This ADR provides the mechanism; it does not mandate its use.

## Blast radius

**Cheap.** Moving a component from editor-only to shipped, or back, is a registration call
and a line in the cook. No saved file changes meaning, because of point 3.

## Consequences

- **Editor concerns compose instead of accumulating.** A new editor-only concern is a new
  component, not a new field in an existing one and not a new mechanism.
- **The rule "a component is either in both or in the editor" has to be visible**, or
  somebody will add an `#if` inside a struct and it will work on their machine. It belongs
  in `voe3d/CLAUDE.md` next to the component rules, in a sentence.
- **Point 3 is a real tolerance and it has a cost**: a scene referencing a component the
  build does not know loads quietly missing data. That is the correct behaviour for editor
  data and the wrong behaviour for game data, and nothing currently distinguishes them. It
  is accepted because the alternative — refusing to load — makes every editor addition a
  breaking change.
- **The cook gains its first real job beyond compiling.** Until now it was *build again*;
  it now also transforms the scene.

## Rejected options and why

**A — `#if` around fields.** Rejected: it makes a component a different shape in the two
builds, which is the one thing ADR-0122 exists to prevent, and every consequence of that is
silent and appears only in the shipped product.

**C — a parallel editor-side store.** Rejected: it reintroduces the special case the
component model exists to remove. Editor data would not be described by ADR-0122, not
serialised by the same path, not inspected by the same code and not undone by the same
undo — four mechanisms where there was one.

## Questions this opens

None. D-234 — how the world enumerates its registered types — already covers the call the
editor needs to find out which components an entity has, editor-only ones included.
