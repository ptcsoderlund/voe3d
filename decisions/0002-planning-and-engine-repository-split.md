# 0002. Planning and engine are separate repositories, engine entering planning as a submodule

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0001 settles that the engine's own modules share one repository. It does not
settle whether planning material — ideas, roadmap, kanban, research, the
decision register — shares that repository with the engine source.

These two bodies of work have different lifecycles, different audiences, and
different churn. Planning material changes daily and is read by humans and
planning agents; engine source changes against a build and is read by compilers,
CI, and contributors.

## Options considered

### Option A — One repository for everything
Planning material as a `docs/` tree inside the engine repository.

### Option B — Two repositories, engine standalone and unaware
Planning repository references the engine only by URL and prose.

### Option C — Two repositories, engine entering planning as a git submodule
The planning repository is the working root; the engine is checked out beneath
it at a pinned commit.

## Decision

**Option C**, as directed by the principal. The planning repository is the outer
working root and carries the engine repository as a submodule.

Critically, this is a submodule between **planning and code**, not between
**code and code**. The objection recorded in ADR-0001 against submodules does
not transfer: CMake never traverses this boundary, no build depends on the
submodule being current, and no cross-module refactor is split across it.

## Blast radius

**Reversibility: moderate.** The engine repository is fully independent, so the
planning repository can be discarded, re-pointed, or absorbed at any time
without touching engine history. The expensive direction would be merging the
two histories later, which this decision does not require.

## Consequences

- **The onboarding invariant survives only because of an accompanying rule:**
  nobody builds from the planning repository. Engine contributors and CI clone
  the engine repository directly. The submodule is a convenience pointer for
  planning and agent work, never a build path. Without this rule, submodule
  staleness would reintroduce exactly the failure ADR-0001 rejected.
- The submodule pointer will routinely lag the engine's real HEAD. This is
  expected and is not a defect — it is not a version pin that anything builds
  against.
- Agents and humans working at the planning root must not commit inside the
  submodule while it is in detached HEAD. Checking out a branch inside the
  engine repository before editing is mandatory, and belongs in the planning
  repository's `CLAUDE.md`.
- Handing off, open-sourcing, or transferring the engine later is a clean cut,
  because the engine repository carries no planning material.

## Rejected options and why

- **Option A** — rejected because daily planning churn would dominate engine
  history, and because engine handoff or open-sourcing would then require
  disentangling private planning notes from source.
- **Option B** — rejected as strictly worse than C for no gain. Without the
  submodule, an agent or human at the planning root has no path to the code, so
  every cross-cutting task needs two checkouts coordinated by hand. C degrades
  to B simply by ignoring the submodule, so C dominates.

## Questions this opens

- **D-015** — which artifacts live in which repository, and where agents run.
- **D-016** — where the engine repository is hosted, and its initial creation.
