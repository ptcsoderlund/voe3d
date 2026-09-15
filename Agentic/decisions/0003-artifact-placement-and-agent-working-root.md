# 0003. The engine repository is self-sufficient; planning root holds ideation only

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0002 established two repositories. It did not establish which artifacts live
in which, nor where an agent should run.

The governing test: **can someone holding only the engine clone write correct
code and understand why the code is shaped as it is?** The engine repository is
the artifact that gets cloned alone — by CI, by a new contributor, and by any
agent doing code work — and per ADR-0002 it may not depend on anything above it.

## Options considered

### Option A — Engine repository is self-sufficient
Engine carries source, build, CI, `CLAUDE.md`, the ADR sequence and the work
board. Planning carries ideation and the decision register.

### Option B — Planning repository owns all prose
Engine carries source, build, CI and a thin `CLAUDE.md`; every written artifact
lives in planning.

### Option C — Split with guidelines synchronised into both repositories
Two sources of truth kept aligned by tooling.

## Decision

**Option A**, with the placement fixed as follows.

**Engine repository** (`voe3d`):

| Artifact | Path | Why here |
|---|---|---|
| Source, build, CI | `src/`, `CMakeLists.txt`, `.github/` | Obvious. |
| Binding project rules | `CLAUDE.md` | Must reach every contributor and every agent holding only this clone. |
| Architecture decisions | `docs/adr/` | "Why is this like this" must be answerable from the code alone. |
| Work board | `kanban/{todo,review,complete}/` | A card is closeable in the same commit as the code that closes it. |

**Planning repository** (this root):

| Artifact | Path | Why here |
|---|---|---|
| Ideation, brainstorming, research | `docs/prestudy/`, notes | Daily churn; must not pollute engine history. |
| Decision register | `docs/prestudy/decision-register.md` | Tracks *process state*, which is planning's job, not the engine's. |
| Agent working rules | `CLAUDE.md` | Governs navigation between the two repositories. |
| The engine | `voe3d/` (submodule) | Per ADR-0002. |

**Claude skills live in user space** (`~/.claude/skills/`), in neither
repository, as directed by the principal.

**Workflow.** A decision is taken and recorded as an ADR in the engine
repository. It then enters `kanban/todo/` as a card. Implementation moves the
card to `review/`, then `complete/`. Ideation upstream of a decision happens in
the planning root and never produces a card directly.

**Agent working root.** Code work runs with the working directory inside the
engine repository. Planning, ideation and decision work runs at the planning
root. An agent at the planning root does not edit code through the submodule
path.

## Blast radius

**Reversibility: cheap now, moderate later.** Relocating a documentation folder
between the two repositories is a move while histories are short. It becomes
moderately expensive once kanban cards and ADRs have accumulated history worth
preserving, since preserving it then requires `git filter-repo`.

## Consequences

- **Skills are not versioned with the code and are not delivered by a clone.** A
  new contributor, and CI, receive none of them. The governing rule that follows:
  **anything that must bind all contributors belongs in the engine's
  `CLAUDE.md`; a skill may only accelerate work, never define a convention that
  correctness depends on.** Accepted knowingly.
- Recording a decision touches both repositories: the ADR in the engine, the
  register row in planning. Cross-repository links in the register resolve
  through the submodule path.
- The board has **no in-progress bucket**. Two agents can therefore claim the
  same card from `todo/` without a visible conflict. Mitigated for now by a
  `status:` field inside the card; revisit if parallel agents become routine.
- The engine repository can be handed off, open-sourced or transferred as a
  clean unit, carrying its rationale and rules with it.

## Rejected options and why

- **Option B** — rejected because the engine clone could then answer neither
  "how do I write code here" nor "why is this module here", and because an agent
  working inside the submodule would need the parent checked out to load its own
  rules, inverting the submodule direction established in ADR-0002.
- **Option C** — rejected on sight. It trades one cross-repository link for two
  sources of truth and silent drift, which is the worse failure.
- **Variant: ADRs kept in planning alongside the register** — rejected because
  it defeats the self-sufficiency test that decided this ADR. The register is
  process state and stays; the ADRs are rationale for the code and travel with
  it.

## Questions this opens

- **D-017** — does the board need an in-progress bucket or a claim mechanism
  once more than one agent works concurrently?
