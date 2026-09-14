# 0010. glTF is an import format; authoring is text, shipping is binary

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal requires authoring data to be **as text-based as possible, so
future AI agents can take part in developing games** with the engine. Shipping a
game then goes through a build step producing binary.

This is the requirement that promoted the editor from non-goal to constraint
(ADR-0009), because it reaches into the ECS.

Recorded as Proposed pending one scope question — whether glTF also carries
entity and component data. **Now resolved: it does not.**

## Decision

**glTF is an import format only, for models, animations and materials.** No
FBX, no OBJ, no proprietary mesh format. Text form (`.gltf`) over `.glb`,
consistent with the text-first requirement.

**glTF is not a scene format and is not asked to be one.** It describes meshes,
materials and animation; it has no concept of an entity or a component, and
ADR-0007 makes the world out of those. Component data is *not* smuggled into
glTF `extras`.

**Making things happen requires the engine's API** — either code written against
it, or later the editor. A glTF file is content, not a game.

**Two representations, one conversion:**

| Stage | Form | Why |
|---|---|---|
| Authoring / editing | **Text** | Diffable, reviewable, writable by an agent or by hand |
| Shipping | **Binary**, via a build step | Load speed and size; no parse cost at runtime |

The build step is **later**, after the renderer (ADR-0009). The runtime is
nonetheless written with loading behind a seam from the start, so the binary
path is an addition rather than a retrofit.

## Blast radius

**Reversibility: moderate.** Changing the authoring format later is a converter
plus a re-export. Changing the decision to *have* a text authoring form is
load-bearing — ADR-0007's component design depends on it.

## Consequences

- **Components must be text-serialisable** — the reflection-shaped requirement,
  D-024. A constraint on the ECS from day one, even though no editor exists.
- **v1 has no scene file.** With no editor yet, scenes are constructed in code
  against the engine API. The text scene format arrives with the editor; what
  matters now is that components stay describable enough for it to be possible.
- Two loading paths eventually exist, text and binary, and must produce
  identical runtime state. That equivalence needs a test the day the binary path
  appears.
- The engine depends on a glTF parser — a third-party dependency (D-004) that is
  now certain rather than hypothetical, and the first real test of the
  onboarding invariant after Vulkan.
- **The asset pipeline is no longer a non-goal.** Deferred work with a known
  shape. Tracked as D-025.

## Rejected options and why

- **Component data in glTF `extras`** — rejected. `extras` is untyped,
  unvalidated and invisible to every glTF tool, so it buys one file extension
  and pays by making the game's own data second-class inside the format chosen
  precisely for being standard.
- **A second import format** (FBX, OBJ) — rejected. One format, imported well,
  beats two imported adequately, and every additional parser is another
  dependency weighed against the onboarding invariant.

## Questions this opens

- **D-024** — component type description for text serialisation.
- **D-025** — the text-to-binary build step.
- **D-026** — which glTF parser, and does it satisfy the onboarding invariant?
