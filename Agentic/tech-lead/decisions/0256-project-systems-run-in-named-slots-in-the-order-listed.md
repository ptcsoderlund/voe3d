# 0256 — A project's systems run in named slots, in the order the project lists them
date: 2026-09-26
by: tech-lead

## Decision
A project places each of its systems in one of two named slots of the fixed step (0254): **before
the move** (where every project system runs today) and **after the move** (new: after the bodies'
move and the transform system, still inside the same step, so what it moves is remembered and
drawn at the same lag as everything else — the engine's LateUpdate/PostUpdate). A project
without after-the-move systems declares none and behaves as now. Within a slot, systems run in
the order the project lists them; that list order is the only ordering a project can ask for.
There are no before/after rules, priorities or scheduler. Following a moved thing (a camera on
the capsule) is an after-the-move system. The example project's camera follow moves there, which
closes 027 bug 01. **The door to threads stays open:** a slot is a phase boundary, and anything
threaded later must finish one slot before the next starts. Systems still talk only through data
and intents (ADR-0017), never by assuming what another system has just done in the same slot
beyond list order. If a threading decision ever runs a slot's systems in parallel, it replaces list
order with explicit ordering for the systems that need it (Bevy's `.before`/`.after`), and it
makes that change, not this one.

## Reasoning
One thread (ADR-0065) already gives a fixed, repeatable order, so ordering rules would pin
something that is never loose. The real need, running after the move, is a slot, which is also how
Bevy places a camera follow (`PostUpdate`). Alternatives: before/after rules now (a scheduler for a
problem one thread does not have, ADR-0008); fixing it in the example alone (every game would copy
the workaround).

## Replaces
nothing. Amends 0254's step order: the project's after-the-move systems run last in a step.
