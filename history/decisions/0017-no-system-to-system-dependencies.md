# 0017. Systems do not depend on systems; intent is submitted as data

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amends:** the intent *delivery mechanism* of ADR-0011. ADR-0011's ownership
  rule and ADR-0013's typed-intent decision stand unchanged.

## Context

The principal: *"We don't want system -> system -> system dependencies."*

This exposes a defect in ADR-0011, which recorded intent as "a function call on
the owning module's public API" and marked it cheap to reverse per seam. That
mechanism produces exactly the chains now ruled out: if `v_physics_system` calls
`v_transform_system_request_move()`, physics links against transform's system,
and three such hops are a chain.

The stated rule is not compatible with the recorded mechanism. The rule wins.

## Decision

**A system depends on data, never on another system.** Its own component, the
components it reads, and intent types. Nothing else.

**Intent is submitted as data, through the ECS.** A system that wants a change
constructs an intent value and hands it to the ECS. The owning system drains
intents of that type and applies them. The submitter never names the system that
will act.

This is what "intent is a datatype" (ADR-0013) was always pointing at; ADR-0011
had not yet drawn the consequence.

## Consequences

- **The dependency graph flattens.** Edges run system → data, never system →
  system. A system's link dependencies are component headers and intent type
  declarations — no system implementation is ever a link dependency of another.
- **Intent handling becomes deferred rather than immediate.** Submitting no
  longer performs the change; the owning system applies it when it next runs.
  This is a real semantic change from ADR-0011 and callers must not assume the
  effect is visible on the next line.
- **It opens the ordering question ADR-0011 postponed.** When queues drain, and
  whether an intent submitted this frame is seen this frame or next, now needs
  an answer. Tracked as **D-031**.
- **It solves the threading problem properly rather than deferring it.** Queued
  intents are batchable, orderable and safe to submit from several threads —
  the concurrency gap recorded in ADR-0013 closes as a side effect.
- **Cost: dispatch by intent type.** The ECS routes intent values to drains by
  type id, which is a small runtime mechanism ADR-0011's direct call avoided.
  Accepted as the price of the rule.
- `coding_convention.md` carries the rule; this ADR carries the reasoning.

## Rejected options and why

- **Keeping direct calls and forbidding chains by discipline** — rejected. The
  chain would be structurally possible and prevented only by vigilance, which is
  the failure mode this project has now rejected four times.

## Questions this opens

- **D-031** — when intent queues drain, and whether an intent is visible in the
  frame it was submitted.
