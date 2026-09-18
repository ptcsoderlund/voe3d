# 0011. A component and its system are one module; mutation crosses folders only as intent

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0007 recorded the ownership rule that keeps an ECS compatible with
`guidelines.md`'s ban on outside mutation: *a component type is owned by the
folder that defines it.* That said who owns what. It did not say how anyone else
gets a change made.

The principal specified the mechanism directly: `MyEntity.h` and
`MyEntitySystem.h` are coupled as one module; the system **receives intent and
mutates data based on the intent request**, so mutation stays inside the module.

## Decision

**The unit of ownership is the component-and-system pairing.** A component
declaration and the system that operates on it are one module and are not
separated. The system is the only code that writes that component.

**Writes cross a folder boundary only as intent.** Outside code does not reach
into another module's component fields. It states what it wants; the owning
system decides whether and how the data changes.

**Reads cross boundaries directly, and are read-only.** This is the necessary
other half and is stated explicitly because the rule is unworkable without it —
the renderer must read transforms every frame, and routing reads through intent
would be absurd. `guidelines.md` already provides for this: *"data is exposed as
immutable, read-only or as a copy."*

**Intent is, for now, a function call on the owning module's public API.** The
module's API *is* the intent vocabulary — `foo_request_x(world, entity, …)` —
rather than a queue, a message bus or a command buffer.

**Reversibility: cheap, per seam, and deliberately so.** Upgrading one specific
seam to a queued or deferred form later is a local change behind an unchanged
API shape. A queued intent system buys deterministic ordering, batching and
replay; none is a v1 requirement, and building it now would repeat the premature
generality ADR-0008 rejected. This is marked as a fast decision, not a
load-bearing one.

## Consequences

- **A component is defined by the module that writes it.** This falls out of the
  rule and is the practical heuristic for placing a new component. Two modules
  that both want to write conceptually similar data means either two components
  or one intent flow — never a shared field with two writers.
- **The transform problem is solved by construction.** Animation, physics and
  gameplay all wanting to move things is the classic ECS write-conflict.
  Here transforms have exactly one writer and everyone else sends intent, so
  "who moved this, and in what order" is answerable rather than emergent.
- **Cost is at boundaries only.** Within a module the system writes its
  component arrays directly, in a tight loop, with no indirection. The intent
  protocol is a folder-boundary rule, never an inner-loop one — so the usual
  performance objection to message-passing does not apply here.
- **Folders are groups of component-system pairings**, chosen by function per
  `guidelines.md`. This substantially pre-shapes the module map before it is
  drawn (D-002).
- **A pairing is larger than a header+source pair.** The engine's `CLAUDE.md`
  maps "module" onto `foo.h` + `foo.c`; the ownership unit here spans two such
  pairs. That wording needs updating in the engine repository — folded into
  D-021, which is already blocked on the repository existing.
- Intent handlers are a place where behaviour can hide from the editor. Per
  ADR-0009, anything that must be editable later is component data, not state
  private to a system.

## Rejected options and why

- **Systems write whatever components they query**, as most ECS frameworks
  allow — rejected because it contradicts `guidelines.md` outright and
  reintroduces exactly the multiple-writer ambiguity this decision removes.
- **A queued intent bus now** — rejected as premature generality. It serves
  determinism and replay, neither of which has a consumer yet, and the seam can
  be upgraded later without changing call sites.

## Questions this opens

None blocking. The map of folders (D-002) is now substantially determined and is
the next decision.
